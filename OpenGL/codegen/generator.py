#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from collections import defaultdict
from dataclasses import dataclass, field
from enum import Flag, IntFlag
from typing import Any



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

# TODO: Remove this WA when Python 3.10 and older are no longer supported.
def ensure_flag_class_is_iterable(flag_class) -> None:
    def intflag_iter(self: Flag):
        """
        Yield flags set in a value.

        Since Python 3.11, Flag and IntFlag are iterable. For earlier versions,
        set their __iter__ to this.
        """
        for member in self.__class__:
            nonzero: bool = member.value != 0
            power_of_two: bool = (member.value & (member.value - 1) == 0)
            if nonzero and power_of_two and member in self:
                yield member


    # Make flag iterable if it isn't already.
    if not hasattr(flag_class, '__iter__'):
        print(f"Adding missing __iter__ to {flag_class}")
        flag_class.__iter__ = intflag_iter
        return

    # In Python 3.10 on Windows, there is IntFlag.__iter__ that iterates over
    # variants in the flag class instead of iterating over bits set in a flag
    # instance. This is useless to us, so we overwrite existing __iter__.
    problem_detected: bool = False

    class TestFlag(flag_class):
        """For checking __iter__ on flag instances."""

        NONE = 0
        FOO = 1
        BAR = 2
        BAZ = 4

    test_instance = TestFlag.FOO | TestFlag.BAZ
    checklist = [TestFlag.FOO, TestFlag.BAZ]
    try:
        for bit in test_instance:
            checklist.remove(bit)
    except TypeError:
        problem_detected = True
    problem_detected = problem_detected or len(checklist) != 0
    if problem_detected:
        print(f"Patching problematic __iter__ in {flag_class}")
        flag_class.__iter__ = intflag_iter

ensure_flag_class_is_iterable(IntFlag)


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

    # We no longer need inherit_from and we don't store it in Token.
    for funcs in functions_dict.values():
        for func in funcs:
            func.pop('inherit_from', None)


def function(**kwargs) -> None:
    """Process function data and save it to a dict."""
    name: str = kwargs['name']
    functions_dict[name].append(kwargs)


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


function(name='glAccum', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='op', type='GLenum'),
        Argument(name='value', type='GLfloat'),
    ],
)

function(name='glAccumxOES', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='op', type='GLenum'),
        Argument(name='value', type='GLfixed'),
    ],
)

function(name='glAcquireKeyedMutexWin32EXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='memory', type='GLuint'),
        Argument(name='key', type='GLuint64'),
        Argument(name='timeout', type='GLuint'),
    ],
)

function(name='glActiveProgramEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

function(name='glActiveShaderProgram', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pipeline', type='GLuint', wrap_type='CGLPipeline'),
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

function(name='glActiveShaderProgramEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glActiveShaderProgram')

function(name='glActiveStencilFaceEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
    ],
)

function(name='glActiveTexture', enabled=True, function_type=FuncType.PARAM, state_track=True, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
    ],
)

function(name='glActiveTextureARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glActiveTexture')

function(name='glActiveVaryingNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

function(name='glAddSwapHintRectWIN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

function(name='glAlphaFragmentOp1ATI', enabled=True, function_type=FuncType.PARAM,
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

function(name='glAlphaFragmentOp2ATI', enabled=True, function_type=FuncType.PARAM,
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

function(name='glAlphaFragmentOp3ATI', enabled=True, function_type=FuncType.PARAM,
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

function(name='glAlphaFunc', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='func', type='GLenum'),
        Argument(name='ref', type='GLfloat'),
    ],
)

function(name='glAlphaFuncQCOM', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='func', type='GLenum'),
        Argument(name='ref', type='GLclampf'),
    ],
)

function(name='glAlphaFuncx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='func', type='GLenum'),
        Argument(name='ref', type='GLfixed'),
    ],
)

function(name='glAlphaFuncxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glAlphaFuncx')

function(name='glAlphaToCoverageDitherControlNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glApplyFramebufferAttachmentCMAAINTEL', enabled=True, function_type=FuncType.FILL,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glApplyFramebufferAttachmentCmaaINTEL', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target',type='GLenum'),
        Argument(name='attachment',type='GLenum'),
    ],
)

function(name='glApplyTextureEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glAreProgramsResidentNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='programs', type='const GLuint*'),
        Argument(name='residences', type='GLboolean*'),
    ],
)

function(name='glAreTexturesResident', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='textures', type='const GLuint*'),
        Argument(name='residences', type='GLboolean*'),
    ],
)

function(name='glAreTexturesResidentEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glAreTexturesResident')

function(name='glArrayElement', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(i)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='i', type='GLint'),
    ],
)

function(name='glArrayElementEXT', enabled=True, function_type=FuncType.RENDER, inherit_from='glArrayElement')

function(name='glArrayObjectATI', enabled=False, function_type=FuncType.NONE,
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

function(name='glAsyncMarkerSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='marker', type='GLuint'),
    ],
)

function(name='glAttachObjectARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='containerObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='obj', type='GLhandleARB', wrap_type='CGLProgram'),
    ],
)

function(name='glAttachShader', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
    ],
)

function(name='glBegin', enabled=True, function_type=FuncType.PARAM, run_wrap=True, pass_token=True, state_track=True, prefix="""  // glBegin/glEnd are special, in that flattened playback in one thread
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

function(name='glBeginConditionalRender', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glBeginConditionalRenderNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glBeginConditionalRender')

function(name='glBeginConditionalRenderNVX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
    ],
)

function(name='glBeginFragmentShaderATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glBeginOcclusionQueryNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
    ],
)

function(name='glBeginPerfMonitorAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='monitor', type='GLuint'),
    ],
)

function(name='glBeginPerfQueryINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='queryHandle', type='GLuint'),
    ],
)

function(name='glBeginQuery', enabled=True, function_type=FuncType.PARAM|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='id', type='GLuint'),
    ],
)

function(name='glBeginQueryARB', enabled=True, function_type=FuncType.PARAM|FuncType.QUERY, inherit_from='glBeginQuery')

function(name='glBeginQueryEXT', enabled=True, function_type=FuncType.PARAM|FuncType.QUERY, inherit_from='glBeginQuery')

function(name='glBeginQueryIndexed', enabled=True, function_type=FuncType.PARAM|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='id', type='GLuint'),
    ],
)

function(name='glBeginTransformFeedback', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='primitiveMode', type='GLenum'),
    ],
)

function(name='glBeginTransformFeedbackEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glBeginTransformFeedback')

function(name='glBeginTransformFeedbackNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glBeginTransformFeedback')

function(name='glBeginVertexShaderEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glBeginVideoCaptureNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
    ],
)

function(name='glBindAttribLocation', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='index', type='GLuint'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

function(name='glBindAttribLocationARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glBindAttribLocation')

function(name='glBindBuffer', enabled=True, function_type=FuncType.BIND, state_track=True, rec_condition='ConditionBufferES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

function(name='glBindBufferARB', enabled=True, function_type=FuncType.BIND, inherit_from='glBindBuffer')

function(name='glBindBufferBase', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

function(name='glBindBufferBaseEXT', enabled=True, function_type=FuncType.BIND, inherit_from='glBindBufferBase')

function(name='glBindBufferBaseNV', enabled=True, function_type=FuncType.BIND, inherit_from='glBindBufferBase')

function(name='glBindBufferOffsetEXT', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
    ],
)

function(name='glBindBufferOffsetNV', enabled=True, function_type=FuncType.BIND, inherit_from='glBindBufferOffsetEXT')

function(name='glBindBufferRange', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
    ],
)

function(name='glBindBufferRangeEXT', enabled=True, function_type=FuncType.BIND, inherit_from='glBindBufferRange')

function(name='glBindBufferRangeNV', enabled=True, function_type=FuncType.BIND, inherit_from='glBindBufferRange')

function(name='glBindBuffersBase', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='buffers', type='const GLuint*', wrap_type='CGLBuffer::CSArray', wrap_params='count, buffers'),
    ],
)

function(name='glBindBuffersRange', enabled=False, function_type=FuncType.NONE,
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

function(name='glBindFragDataLocation', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='color', type='GLuint'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

function(name='glBindFragDataLocationEXT', enabled=True, function_type=FuncType.BIND, inherit_from='glBindFragDataLocation')

function(name='glBindFragDataLocationIndexed', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='colorNumber', type='GLuint'),
        Argument(name='index', type='GLuint'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

function(name='glBindFragDataLocationIndexedEXT', enabled=False, function_type=FuncType.BIND, inherit_from='glBindFragDataLocationIndexed')

function(name='glBindFragmentShaderATI', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
    ],
)

function(name='glBindFramebuffer', enabled=True, function_type=FuncType.BIND, state_track=True, run_wrap=True, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
    ],
)

function(name='glBindFramebufferEXT', enabled=True, function_type=FuncType.BIND, state_track=True, run_wrap=False, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebufferEXT'),
    ],
)

function(name='glBindFramebufferOES', enabled=True, function_type=FuncType.BIND, inherit_from='glBindFramebufferEXT', run_wrap=False, recorder_wrap=True)

function(name='glBindImageTexture', enabled=True, function_type=FuncType.BIND,
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

function(name='glBindImageTextureEXT', enabled=True, function_type=FuncType.BIND,
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

function(name='glBindImageTextures', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='textures', type='const GLuint*', wrap_type='CGLTexture::CSArray', wrap_params='count, textures'),
    ],
)

function(name='glBindLightParameterEXT', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='value', type='GLenum'),
    ],
)

function(name='glBindMaterialParameterEXT', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='value', type='GLenum'),
    ],
)

function(name='glBindMultiTextureEXT', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
    ],
)

function(name='glBindParameterEXT', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='value', type='GLenum'),
    ],
)

function(name='glBindProgramARB', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='program', type='GLuint'),
    ],
)

function(name='glBindProgramNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='id', type='GLuint'),
    ],
)

function(name='glBindProgramPipeline', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pipeline', type='GLuint', wrap_type='CGLPipeline'),
    ],
)

function(name='glBindProgramPipelineEXT', enabled=True, function_type=FuncType.BIND, inherit_from='glBindProgramPipeline')

function(name='glBindRenderbuffer', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
    ],
)

function(name='glBindRenderbufferEXT', enabled=True, function_type=FuncType.BIND, state_track=True, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbufferEXT'),
    ],
)

function(name='glBindRenderbufferOES', enabled=True, function_type=FuncType.BIND, inherit_from='glBindRenderbuffer')

function(name='glBindSampler', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='unit', type='GLuint'),
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
    ],
)

function(name='glBindSamplers', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='samplers', type='const GLuint*', wrap_type='CGLSampler::CSArray', wrap_params='count, samplers'),
    ],
)

function(name='glBindTexGenParameterEXT', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='unit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='value', type='GLenum'),
    ],
)

function(name='glBindTexture', enabled=True, function_type=FuncType.BIND, state_track=True, rec_condition='ConditionTextureES(_recorder)', run_condition='ConditionCurrentContextZero()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
    ],
)

function(name='glBindTextureEXT', enabled=True, function_type=FuncType.BIND, inherit_from='glBindTexture', rec_condition=False, run_condition=False)

function(name='glBindTextureUnit', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='unit', type='GLuint'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
    ],
)

function(name='glBindTextureUnitParameterEXT', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='unit', type='GLenum'),
        Argument(name='value', type='GLenum'),
    ],
)

function(name='glBindTextures', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='textures', type='const GLuint*', wrap_type='CGLTexture::CSArray', wrap_params='count, textures'),
    ],
)

function(name='glBindTransformFeedback', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='id', type='GLuint', wrap_type='CGLTransformFeedback'),
    ],
)

function(name='glBindTransformFeedbackNV', enabled=True, function_type=FuncType.BIND, inherit_from='glBindTransformFeedback')

function(name='glBindVertexArray', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLuint', wrap_type='CGLVertexArray'),
    ],
)

function(name='glBindVertexArrayAPPLE', enabled=False, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLuint'),
    ],
)

function(name='glBindVertexArrayOES', enabled=True, function_type=FuncType.BIND, inherit_from='glBindVertexArray')

function(name='glBindVertexBuffer', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='bindingindex', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='stride', type='GLsizei'),
    ],
)

function(name='glBindVertexBuffers', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='buffers', type='const GLuint*'),
        Argument(name='offsets', type='const GLintptr*'),
        Argument(name='strides', type='const GLsizei*'),
    ],
)

function(name='glBindVertexShaderEXT', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
    ],
)

function(name='glBindVideoCaptureStreamBufferNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='stream', type='GLuint'),
        Argument(name='frame_region', type='GLenum'),
        Argument(name='offset', type='GLintptrARB'),
    ],
)

function(name='glBindVideoCaptureStreamTextureNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='stream', type='GLuint'),
        Argument(name='frame_region', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
    ],
)

function(name='glBinormal3bEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='bx', type='GLbyte'),
        Argument(name='by', type='GLbyte'),
        Argument(name='bz', type='GLbyte'),
    ],
)

function(name='glBinormal3bvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLbyte*'),
    ],
)

function(name='glBinormal3dEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='bx', type='GLdouble'),
        Argument(name='by', type='GLdouble'),
        Argument(name='bz', type='GLdouble'),
    ],
)

function(name='glBinormal3dvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*'),
    ],
)

function(name='glBinormal3fEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='bx', type='GLfloat'),
        Argument(name='by', type='GLfloat'),
        Argument(name='bz', type='GLfloat'),
    ],
)

function(name='glBinormal3fvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glBinormal3iEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='bx', type='GLint'),
        Argument(name='by', type='GLint'),
        Argument(name='bz', type='GLint'),
    ],
)

function(name='glBinormal3ivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*'),
    ],
)

function(name='glBinormal3sEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='bx', type='GLshort'),
        Argument(name='by', type='GLshort'),
        Argument(name='bz', type='GLshort'),
    ],
)

function(name='glBinormal3svEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*'),
    ],
)

function(name='glBinormalPointerEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glBitmap', enabled=True, function_type=FuncType.RESOURCE,
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

function(name='glBitmapxOES', enabled=False, function_type=FuncType.NONE,
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

function(name='glBlendBarrier', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glBlendBarrierKHR', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendBarrier')

function(name='glBlendBarrierNV', enabled=False, function_type=FuncType.NONE, inherit_from='glBlendBarrier')

function(name='glBlendColor', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfloat'),
        Argument(name='green', type='GLfloat'),
        Argument(name='blue', type='GLfloat'),
        Argument(name='alpha', type='GLfloat'),
    ],
)

function(name='glBlendColorEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendColor')

function(name='glBlendColorxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfixed'),
        Argument(name='green', type='GLfixed'),
        Argument(name='blue', type='GLfixed'),
        Argument(name='alpha', type='GLfixed'),
    ],
)

function(name='glBlendEquation', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glBlendEquationEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquation')

function(name='glBlendEquationIndexedAMD', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquationi')

function(name='glBlendEquationOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquation')

function(name='glBlendEquationSeparate', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='modeRGB', type='GLenum'),
        Argument(name='modeAlpha', type='GLenum'),
    ],
)

function(name='glBlendEquationSeparateATI', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquationSeparate')

function(name='glBlendEquationSeparateEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquationSeparate')

function(name='glBlendEquationSeparateIndexedAMD', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquationSeparatei')

function(name='glBlendEquationSeparateOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquationSeparate')

function(name='glBlendEquationSeparatei', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buf', type='GLuint'),
        Argument(name='modeRGB', type='GLenum'),
        Argument(name='modeAlpha', type='GLenum'),
    ],
)

function(name='glBlendEquationSeparateiARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquationSeparatei')

function(name='glBlendEquationSeparateiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquationSeparatei')

function(name='glBlendEquationSeparateiOES', enabled=False, function_type=FuncType.NONE, inherit_from='glBlendEquationSeparatei')

function(name='glBlendEquationi', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buf', type='GLuint'),
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glBlendEquationiARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquationi')

function(name='glBlendEquationiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendEquationi')

function(name='glBlendEquationiOES', enabled=False, function_type=FuncType.NONE, inherit_from='glBlendEquationi')

function(name='glBlendFunc', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sfactor', type='GLenum'),
        Argument(name='dfactor', type='GLenum'),
    ],
)

function(name='glBlendFuncIndexedAMD', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buf', type='GLuint'),
        Argument(name='src', type='GLenum'),
        Argument(name='dst', type='GLenum'),
    ],
)

function(name='glBlendFuncSeparate', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sfactorRGB', type='GLenum'),
        Argument(name='dfactorRGB', type='GLenum'),
        Argument(name='sfactorAlpha', type='GLenum'),
        Argument(name='dfactorAlpha', type='GLenum'),
    ],
)

function(name='glBlendFuncSeparateEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendFuncSeparate')

function(name='glBlendFuncSeparateINGR', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendFuncSeparate')

function(name='glBlendFuncSeparateIndexedAMD', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendFuncSeparatei')

function(name='glBlendFuncSeparateOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendFuncSeparate')

function(name='glBlendFuncSeparatei', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buf', type='GLuint'),
        Argument(name='srcRGB', type='GLenum'),
        Argument(name='dstRGB', type='GLenum'),
        Argument(name='srcAlpha', type='GLenum'),
        Argument(name='dstAlpha', type='GLenum'),
    ],
)

function(name='glBlendFuncSeparateiARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendFuncSeparatei')

function(name='glBlendFuncSeparateiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendFuncSeparatei')

function(name='glBlendFuncSeparateiOES', enabled=False, function_type=FuncType.NONE, inherit_from='glBlendFuncSeparatei')

function(name='glBlendFunci', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buf', type='GLuint'),
        Argument(name='src', type='GLenum'),
        Argument(name='dst', type='GLenum'),
    ],
)

function(name='glBlendFunciARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendFunci')

function(name='glBlendFunciEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glBlendFunci')

function(name='glBlendFunciOES', enabled=False, function_type=FuncType.NONE, inherit_from='glBlendFunci')

function(name='glBlendParameteriNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLint'),
    ],
)

function(name='glBlitFramebuffer', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
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

function(name='glBlitFramebufferANGLE', enabled=True, function_type=FuncType.FILL, inherit_from='glBlitFramebuffer')

function(name='glBlitFramebufferEXT', enabled=True, function_type=FuncType.FILL, inherit_from='glBlitFramebuffer')

function(name='glBlitFramebufferNV', enabled=False, function_type=FuncType.FILL, inherit_from='glBlitFramebuffer')

function(name='glBlitNamedFramebuffer', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
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

function(name='glBufferAddressRangeNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='address', type='GLuint64EXT'),
        Argument(name='length', type='GLsizeiptr'),
    ],
)

function(name='glBufferData', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionBufferData(_recorder, target, size, data, usage)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, size'),
        Argument(name='usage', type='GLenum'),
    ],
)

function(name='glBufferDataARB', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glBufferData', rec_condition=False)

function(name='glBufferPageCommitmentARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='commit', type='GLboolean'),
    ],
)

function(name='glBufferParameteriAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glBufferStorage', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionBufferStorage(_recorder, target, size, data, flags)', interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, size'),
        Argument(name='flags', type='GLbitfield', wrap_type='CBufferStorageFlags'),
    ],
)

function(name='glBufferStorageEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glBufferStorage', interceptor_exec_override=False)

function(name='glBufferStorageExternalEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='clientBuffer', type='GLeglClientBufferEXT'),
        Argument(name='flags', type='GLbitfield'),
    ],
)

function(name='glBufferStorageMemEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='memory', type='GLuint'),
        Argument(name='offset', type='GLuint64'),
    ],
)

function(name='glBufferSubData', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionBufferES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, size'),
    ],
)

function(name='glBufferSubDataARB', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glBufferSubData', rec_condition=False)

function(name='glCallCommandListNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
    ],
)

function(name='glCallList', enabled=True, function_type=FuncType.RENDER, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
    ],
)

function(name='glCallLists', enabled=True, function_type=FuncType.RENDER, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='type', type='GLenum'),
        Argument(name='lists', type='const void*', wrap_type='CGLubyte::CSArray', wrap_params='DataTypeSize(type) * n, static_cast<const GLubyte*>(lists)'),
    ],
)

function(name='glCheckFramebufferStatus', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

function(name='glCheckFramebufferStatusEXT', enabled=True, function_type=FuncType.GET, inherit_from='glCheckFramebufferStatus', recorder_wrap=True)

function(name='glCheckFramebufferStatusOES', enabled=True, function_type=FuncType.GET, inherit_from='glCheckFramebufferStatus')

function(name='glCheckNamedFramebufferStatus', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='target', type='GLenum'),
    ],
)

function(name='glCheckNamedFramebufferStatusEXT', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebufferEXT'),
        Argument(name='target', type='GLenum'),
    ],
)

function(name='glClampColor', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='clamp', type='GLenum'),
    ],
)

function(name='glClampColorARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glClampColor')

function(name='glClear', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLbitfield'),
    ],
)

function(name='glClearAccum', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfloat'),
        Argument(name='green', type='GLfloat'),
        Argument(name='blue', type='GLfloat'),
        Argument(name='alpha', type='GLfloat'),
    ],
)

function(name='glClearAccumxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfixed'),
        Argument(name='green', type='GLfixed'),
        Argument(name='blue', type='GLfixed'),
        Argument(name='alpha', type='GLfixed'),
    ],
)

function(name='glClearBufferData', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, texelSize(format, type)'),
    ],
)

function(name='glClearBufferSubData', enabled=True, function_type=FuncType.RESOURCE, state_track=True,
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

function(name='glClearBufferfi', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLenum'),
        Argument(name='drawbuffer', type='GLint'),
        Argument(name='depth', type='GLfloat'),
        Argument(name='stencil', type='GLint'),
    ],
)

function(name='glClearBufferfv', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLenum'),
        Argument(name='drawbuffer', type='GLint'),
        Argument(name='value', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='buffer, value'),
    ],
)

function(name='glClearBufferiv', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLenum'),
        Argument(name='drawbuffer', type='GLint'),
        Argument(name='value', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='buffer, value'),
    ],
)

function(name='glClearBufferuiv', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLenum'),
        Argument(name='drawbuffer', type='GLint'),
        Argument(name='value', type='const GLuint*', wrap_type='CGLuint::CSParamArray', wrap_params='buffer, value'),
    ],
)

function(name='glClearColor', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfloat'),
        Argument(name='green', type='GLfloat'),
        Argument(name='blue', type='GLfloat'),
        Argument(name='alpha', type='GLfloat'),
    ],
)

function(name='glClearColorIiEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLint'),
        Argument(name='green', type='GLint'),
        Argument(name='blue', type='GLint'),
        Argument(name='alpha', type='GLint'),
    ],
)

function(name='glClearColorIuiEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLuint'),
        Argument(name='green', type='GLuint'),
        Argument(name='blue', type='GLuint'),
        Argument(name='alpha', type='GLuint'),
    ],
)

function(name='glClearColorx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfixed'),
        Argument(name='green', type='GLfixed'),
        Argument(name='blue', type='GLfixed'),
        Argument(name='alpha', type='GLfixed'),
    ],
)

function(name='glClearColorxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glClearColorx')

function(name='glClearDepth', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='depth', type='GLdouble'),
    ],
)

function(name='glClearDepthdNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='depth', type='GLdouble'),
    ],
)

function(name='glClearDepthf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='d', type='GLfloat'),
    ],
)

function(name='glClearDepthfOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glClearDepthf',
    args=[
        Argument(name='depth', type='GLclampf'),
    ],
)

function(name='glClearDepthx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='depth', type='GLfixed'),
    ],
)

function(name='glClearDepthxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glClearDepthx',
    args=[
        Argument(name='depth', type='GLclampx'),
    ],
)

function(name='glClearIndex', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='GLfloat'),
    ],
)

function(name='glClearNamedBufferData', enabled=True, function_type=FuncType.RESOURCE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, texelSize(format, type)'),
    ],
)

function(name='glClearNamedBufferDataEXT', enabled=False, function_type=FuncType.RESOURCE, inherit_from='glClearNamedBufferData')

function(name='glClearNamedBufferSubData', enabled=True, function_type=FuncType.RESOURCE, state_track=True,
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

function(name='glClearNamedBufferSubDataEXT', enabled=False, function_type=FuncType.RESOURCE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='offset', type='GLsizeiptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, texelSize(format, type)'),
    ],
)

function(name='glClearNamedFramebufferfi', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint'),
        Argument(name='buffer', type='GLenum'),
        Argument(name='drawbuffer', type='GLint'),
        Argument(name='depth', type='GLfloat'),
        Argument(name='stencil', type='GLint'),
    ],
)

function(name='glClearNamedFramebufferfv', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint'),
        Argument(name='buffer', type='GLenum'),
        Argument(name='drawbuffer', type='GLint'),
        Argument(name='value', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='buffer, value'),
    ],
)

function(name='glClearNamedFramebufferiv', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint'),
        Argument(name='buffer', type='GLenum'),
        Argument(name='drawbuffer', type='GLint'),
        Argument(name='value', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='buffer, value'),
    ],
)

function(name='glClearNamedFramebufferuiv', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint'),
        Argument(name='buffer', type='GLenum'),
        Argument(name='drawbuffer', type='GLint'),
        Argument(name='value', type='const GLuint*', wrap_type='CGLuint::CSParamArray', wrap_params='buffer, value'),
    ],
)

function(name='glClearPixelLocalStorageuiEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='offset', type='GLsizei'),
        Argument(name='n', type='GLsizei'),
        Argument(name='values', type='const GLuint*'),
    ],
)

function(name='glClearStencil', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLint'),
    ],
)

function(name='glClearTexImage', enabled=True, function_type=FuncType.RESOURCE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='data', type='const void*', wrap_type='CGLClearTexResource', wrap_params='data, texture, level, format, type'),
    ],
)

function(name='glClearTexImageEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glClearTexImage')

function(name='glClearTexSubImage', enabled=False, function_type=FuncType.NONE,
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

function(name='glClearTexSubImageEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glClearTexSubImage')

function(name='glClientActiveTexture', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
    ],
)

function(name='glClientActiveTextureARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glClientActiveTexture')

function(name='glClientActiveVertexStreamATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
    ],
)

function(name='glClientAttribDefaultEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLbitfield'),
    ],
)

function(name='glClientWaitSync', enabled=True, function_type=FuncType.PARAM, run_wrap=True,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='sync', type='GLsync', wrap_type='CGLsync'),
        Argument(name='flags', type='GLbitfield'),
        Argument(name='timeout', type='GLuint64'),
    ],
)

function(name='glClientWaitSyncAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='sync', type='GLsync'),
        Argument(name='flags', type='GLbitfield'),
        Argument(name='timeout', type='GLuint64'),
    ],
)

function(name='glClipControl', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='origin', type='GLenum'),
        Argument(name='depth', type='GLenum'),
    ],
)

function(name='glClipControlEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glClipControl')

function(name='glClipPlane', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='plane', type='GLenum'),
        Argument(name='equation', type='const GLdouble*', wrap_params='4, equation'),
    ],
)

function(name='glClipPlanef', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='p', type='GLenum'),
        Argument(name='eqn', type='const GLfloat*', wrap_params='4, eqn'),
    ],
)

function(name='glClipPlanefIMG', enabled=True, function_type=FuncType.PARAM, inherit_from='glClipPlanef')

function(name='glClipPlanefOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glClipPlanef')

function(name='glClipPlanex', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='plane', type='GLenum'),
        Argument(name='equation', type='const GLfixed*', wrap_params='4, equation'),
    ],
)

function(name='glClipPlanexIMG', enabled=True, function_type=FuncType.PARAM, inherit_from='glClipPlanex')

function(name='glClipPlanexOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glClipPlanex')

function(name='glColor3b', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLbyte'),
        Argument(name='green', type='GLbyte'),
        Argument(name='blue', type='GLbyte'),
    ],
)

function(name='glColor3bv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLbyte*', wrap_params='3, v'),
    ],
)

function(name='glColor3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLdouble'),
        Argument(name='green', type='GLdouble'),
        Argument(name='blue', type='GLdouble'),
    ],
)

function(name='glColor3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

function(name='glColor3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfloat'),
        Argument(name='green', type='GLfloat'),
        Argument(name='blue', type='GLfloat'),
    ],
)

function(name='glColor3fVertex3fSUN', enabled=True, function_type=FuncType.PARAM,
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

function(name='glColor3fVertex3fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='const GLfloat*', wrap_params='3, c'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

function(name='glColor3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

function(name='glColor3hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLhalfNV'),
        Argument(name='green', type='GLhalfNV'),
        Argument(name='blue', type='GLhalfNV'),
    ],
)

function(name='glColor3hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='3, v'),
    ],
)

function(name='glColor3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLint'),
        Argument(name='green', type='GLint'),
        Argument(name='blue', type='GLint'),
    ],
)

function(name='glColor3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

function(name='glColor3s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLshort'),
        Argument(name='green', type='GLshort'),
        Argument(name='blue', type='GLshort'),
    ],
)

function(name='glColor3sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

function(name='glColor3ub', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLubyte'),
        Argument(name='green', type='GLubyte'),
        Argument(name='blue', type='GLubyte'),
    ],
)

function(name='glColor3ubv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLubyte*', wrap_params='3, v'),
    ],
)

function(name='glColor3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLuint'),
        Argument(name='green', type='GLuint'),
        Argument(name='blue', type='GLuint'),
    ],
)

function(name='glColor3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLuint*', wrap_params='3, v'),
    ],
)

function(name='glColor3us', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLushort'),
        Argument(name='green', type='GLushort'),
        Argument(name='blue', type='GLushort'),
    ],
)

function(name='glColor3usv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLushort*', wrap_params='3, v'),
    ],
)

function(name='glColor3xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfixed'),
        Argument(name='green', type='GLfixed'),
        Argument(name='blue', type='GLfixed'),
    ],
)

function(name='glColor3xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='components', type='const GLfixed*'),
    ],
)

function(name='glColor4b', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLbyte'),
        Argument(name='green', type='GLbyte'),
        Argument(name='blue', type='GLbyte'),
        Argument(name='alpha', type='GLbyte'),
    ],
)

function(name='glColor4bv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLbyte*', wrap_params='4, v'),
    ],
)

function(name='glColor4d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLdouble'),
        Argument(name='green', type='GLdouble'),
        Argument(name='blue', type='GLdouble'),
        Argument(name='alpha', type='GLdouble'),
    ],
)

function(name='glColor4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='4, v'),
    ],
)

function(name='glColor4f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfloat'),
        Argument(name='green', type='GLfloat'),
        Argument(name='blue', type='GLfloat'),
        Argument(name='alpha', type='GLfloat'),
    ],
)

function(name='glColor4fNormal3fVertex3fSUN', enabled=True, function_type=FuncType.PARAM,
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

function(name='glColor4fNormal3fVertex3fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='const GLfloat*', wrap_params='4, c'),
        Argument(name='n', type='const GLfloat*', wrap_params='3, n'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

function(name='glColor4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

function(name='glColor4hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLhalfNV'),
        Argument(name='green', type='GLhalfNV'),
        Argument(name='blue', type='GLhalfNV'),
        Argument(name='alpha', type='GLhalfNV'),
    ],
)

function(name='glColor4hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='4, v'),
    ],
)

function(name='glColor4i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLint'),
        Argument(name='green', type='GLint'),
        Argument(name='blue', type='GLint'),
        Argument(name='alpha', type='GLint'),
    ],
)

function(name='glColor4iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

function(name='glColor4s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLshort'),
        Argument(name='green', type='GLshort'),
        Argument(name='blue', type='GLshort'),
        Argument(name='alpha', type='GLshort'),
    ],
)

function(name='glColor4sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='4, v'),
    ],
)

function(name='glColor4ub', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLubyte'),
        Argument(name='green', type='GLubyte'),
        Argument(name='blue', type='GLubyte'),
        Argument(name='alpha', type='GLubyte'),
    ],
)

function(name='glColor4ubVertex2fSUN', enabled=True, function_type=FuncType.PARAM,
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

function(name='glColor4ubVertex2fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='const GLubyte*', wrap_params='4, c'),
        Argument(name='v', type='const GLfloat*', wrap_params='2, v'),
    ],
)

function(name='glColor4ubVertex3fSUN', enabled=True, function_type=FuncType.PARAM,
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

function(name='glColor4ubVertex3fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='const GLubyte*', wrap_params='4, c'),
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

function(name='glColor4ubv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLubyte*', wrap_params='4, v'),
    ],
)

function(name='glColor4ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLuint'),
        Argument(name='green', type='GLuint'),
        Argument(name='blue', type='GLuint'),
        Argument(name='alpha', type='GLuint'),
    ],
)

function(name='glColor4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLuint*', wrap_params='4, v'),
    ],
)

function(name='glColor4us', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLushort'),
        Argument(name='green', type='GLushort'),
        Argument(name='blue', type='GLushort'),
        Argument(name='alpha', type='GLushort'),
    ],
)

function(name='glColor4usv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLushort*', wrap_params='4, v'),
    ],
)

function(name='glColor4x', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfixed'),
        Argument(name='green', type='GLfixed'),
        Argument(name='blue', type='GLfixed'),
        Argument(name='alpha', type='GLfixed'),
    ],
)

function(name='glColor4xOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glColor4x')

function(name='glColor4xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='components', type='const GLfixed*'),
    ],
)

function(name='glColorFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
    ],
)

function(name='glColorFragmentOp1ATI', enabled=True, function_type=FuncType.PARAM,
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

function(name='glColorFragmentOp2ATI', enabled=True, function_type=FuncType.PARAM,
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

function(name='glColorFragmentOp3ATI', enabled=True, function_type=FuncType.PARAM,
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

function(name='glColorMask', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLboolean'),
        Argument(name='green', type='GLboolean'),
        Argument(name='blue', type='GLboolean'),
        Argument(name='alpha', type='GLboolean'),
    ],
)

function(name='glColorMaskIndexedEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glColorMaski')

function(name='glColorMaski', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='r', type='GLboolean'),
        Argument(name='g', type='GLboolean'),
        Argument(name='b', type='GLboolean'),
        Argument(name='a', type='GLboolean'),
    ],
)

function(name='glColorMaskiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glColorMaski')

function(name='glColorMaskiOES', enabled=False, function_type=FuncType.NONE, inherit_from='glColorMaski')

function(name='glColorMaterial', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glColorP3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='color', type='GLuint'),
    ],
)

function(name='glColorP3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='color', type='const GLuint*', wrap_params='3, color'),
    ],
)

function(name='glColorP4ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='color', type='GLuint'),
    ],
)

function(name='glColorP4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='color', type='const GLuint*', wrap_params='4, color'),
    ],
)

function(name='glColorPointer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

function(name='glColorPointerBounds', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='ptr', type='const GLvoid*', wrap_type='CAttribPtr'),
        Argument(name='count', type='GLsizei'),
    ],
)

function(name='glColorPointerEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='count', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

function(name='glColorPointerListIBM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLint'),
        Argument(name='pointer', type='const void**'),
        Argument(name='ptrstride', type='GLint'),
    ],
)

function(name='glColorPointervINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='pointer', type='const void**'),
    ],
)

function(name='glColorSubTable', enabled=False, function_type=FuncType.NONE,
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

function(name='glColorSubTableEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glColorSubTable')

function(name='glColorTable', enabled=False, function_type=FuncType.NONE,
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

function(name='glColorTableEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glColorTable')

function(name='glColorTableParameterfv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

function(name='glColorTableParameterfvSGI', enabled=False, function_type=FuncType.NONE, inherit_from='glColorTableParameterfv')

function(name='glColorTableParameteriv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glColorTableParameterivSGI', enabled=False, function_type=FuncType.NONE, inherit_from='glColorTableParameteriv')

function(name='glColorTableSGI', enabled=False, function_type=FuncType.NONE, inherit_from='glColorTable')

function(name='glCombinerInputNV', enabled=True, function_type=FuncType.PARAM,
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

function(name='glCombinerOutputNV', enabled=True, function_type=FuncType.PARAM,
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

function(name='glCombinerParameterfNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glCombinerParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

function(name='glCombinerParameteriNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glCombinerParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glCombinerStageParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stage', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

function(name='glCommandListSegmentsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='segments', type='GLuint'),
    ],
)

function(name='glCompileCommandListNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
    ],
)

function(name='glCompileShader', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
    ],
)

function(name='glCompileShaderARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glCompileShader',
    args=[
        Argument(name='shaderObj', type='GLhandleARB', wrap_type='CGLProgram'),
    ],
)

function(name='glCompileShaderIncludeARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='count', type='GLsizei'),
        Argument(name='path', type='const GLchar*const*'),
        Argument(name='length', type='const GLint*'),
    ],
)

function(name='glCompressedMultiTexImage1DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedMultiTexImage2DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedMultiTexImage3DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedMultiTexSubImage1DEXT', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedMultiTexSubImage2DEXT', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedMultiTexSubImage3DEXT', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedTexImage1D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionTextureES(_recorder)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedTexImage1DARB', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glCompressedTexImage1D')

function(name='glCompressedTexImage2D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionTextureES(_recorder)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedTexImage2DARB', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glCompressedTexImage2D', rec_condition=False)

function(name='glCompressedTexImage3D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionTextureES(_recorder)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedTexImage3DARB', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glCompressedTexImage3D')

function(name='glCompressedTexImage3DOES', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glCompressedTexImage3D')

function(name='glCompressedTexSubImage1D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedTexSubImage1DARB', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glCompressedTexSubImage1D')

function(name='glCompressedTexSubImage2D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionTextureES(_recorder)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
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
function(name='glCompressedTexSubImage2DARB', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glCompressedTexSubImage2D', rec_condition=False)

function(name='glCompressedTexSubImage3D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionTextureES(_recorder)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedTexSubImage3DARB', enabled=True, function_type=FuncType.RESOURCE, rec_condition='ConditionTextureES(_recorder)', inherit_from='glCompressedTexSubImage3D')

function(name='glCompressedTexSubImage3DOES', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glCompressedTexSubImage3D')

function(name='glCompressedTextureImage1DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedTextureImage2DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedTextureImage3DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedTextureSubImage1D', enabled=False, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedTextureSubImage1DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedTextureSubImage2D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedTextureSubImage2DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedTextureSubImage3D', enabled=False, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glCompressedTextureSubImage3DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glConservativeRasterParameterfNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLfloat'),
    ],
)

function(name='glConservativeRasterParameteriNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glConvolutionFilter1D', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glConvolutionFilter1DEXT', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glConvolutionFilter1D')

function(name='glConvolutionFilter2D', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glConvolutionFilter2DEXT', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glConvolutionFilter2D')

function(name='glConvolutionParameterf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat'),
    ],
)

function(name='glConvolutionParameterfEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glConvolutionParameterf')

function(name='glConvolutionParameterfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glConvolutionParameterfvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glConvolutionParameterfv')

function(name='glConvolutionParameteri', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint'),
    ],
)

function(name='glConvolutionParameteriEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glConvolutionParameteri')

function(name='glConvolutionParameteriv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glConvolutionParameterivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glConvolutionParameteriv')

function(name='glConvolutionParameterxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

function(name='glConvolutionParameterxvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfixed*'),
    ],
)

function(name='glCopyBufferSubData', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='readTarget', type='GLenum'),
        Argument(name='writeTarget', type='GLenum'),
        Argument(name='readOffset', type='GLintptr'),
        Argument(name='writeOffset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
    ],
)

function(name='glCopyBufferSubDataNV', enabled=False, function_type=FuncType.COPY, inherit_from='glCopyBufferSubData')

function(name='glCopyColorSubTable', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='start', type='GLsizei'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
    ],
)

function(name='glCopyColorSubTableEXT', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyColorSubTable')

function(name='glCopyColorTable', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
    ],
)

function(name='glCopyColorTableSGI', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyColorTable')

function(name='glCopyConvolutionFilter1D', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
    ],
)

function(name='glCopyConvolutionFilter1DEXT', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyConvolutionFilter1D')

function(name='glCopyConvolutionFilter2D', enabled=True, function_type=FuncType.COPY,
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

function(name='glCopyConvolutionFilter2DEXT', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyConvolutionFilter2D')

function(name='glCopyImageSubData', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='srcName', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='srcTarget', type='GLenum'),
        Argument(name='srcLevel', type='GLint'),
        Argument(name='srcX', type='GLint'),
        Argument(name='srcY', type='GLint'),
        Argument(name='srcZ', type='GLint'),
        Argument(name='dstName', type='GLuint', wrap_type='CGLTexture'),
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

function(name='glCopyImageSubDataEXT', enabled=False, function_type=FuncType.COPY, inherit_from='glCopyImageSubData')

function(name='glCopyImageSubDataNV', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyImageSubData')

function(name='glCopyImageSubDataOES', enabled=False, function_type=FuncType.COPY, inherit_from='glCopyImageSubData')

function(name='glCopyMultiTexImage1DEXT', enabled=True, function_type=FuncType.COPY,
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

function(name='glCopyMultiTexImage2DEXT', enabled=True, function_type=FuncType.COPY,
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

function(name='glCopyMultiTexSubImage1DEXT', enabled=True, function_type=FuncType.COPY,
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

function(name='glCopyMultiTexSubImage2DEXT', enabled=True, function_type=FuncType.COPY,
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

function(name='glCopyMultiTexSubImage3DEXT', enabled=True, function_type=FuncType.COPY,
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

function(name='glCopyNamedBufferSubData', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='readBuffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='writeBuffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='readOffset', type='GLintptr'),
        Argument(name='writeOffset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
    ],
)

function(name='glCopyPathNV', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='resultPath', type='GLuint'),
        Argument(name='srcPath', type='GLuint'),
    ],
)

function(name='glCopyPixels', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='type', type='GLenum'),
    ],
)

function(name='glCopyTexImage1D', enabled=True, function_type=FuncType.COPY, state_track=True, rec_condition='ConditionTextureES(_recorder)',
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

function(name='glCopyTexImage1DEXT', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyTexImage1D')

function(name='glCopyTexImage2D', enabled=True, function_type=FuncType.COPY, state_track=True, rec_condition='ConditionTextureES(_recorder)',
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

function(name='glCopyTexImage2DEXT', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyTexImage2D')

function(name='glCopyTexSubImage1D', enabled=True, function_type=FuncType.COPY, rec_condition='ConditionTextureES(_recorder)',
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

function(name='glCopyTexSubImage1DEXT', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyTexSubImage1D')

function(name='glCopyTexSubImage2D', enabled=True, function_type=FuncType.COPY, rec_condition='ConditionTextureES(_recorder)',
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

function(name='glCopyTexSubImage2DEXT', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyTexSubImage2D')

function(name='glCopyTexSubImage3D', enabled=True, function_type=FuncType.COPY, rec_condition='ConditionTextureES(_recorder)',
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

function(name='glCopyTexSubImage3DEXT', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyTexSubImage3D')

function(name='glCopyTexSubImage3DOES', enabled=True, function_type=FuncType.COPY, inherit_from='glCopyTexSubImage3D')

function(name='glCopyTextureImage1DEXT', enabled=True, function_type=FuncType.COPY,
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

function(name='glCopyTextureImage2DEXT', enabled=True, function_type=FuncType.COPY,
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

function(name='glCopyTextureLevelsAPPLE', enabled=False, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='destinationTexture', type='GLuint'),
        Argument(name='sourceTexture', type='GLuint'),
        Argument(name='sourceBaseLevel', type='GLint'),
        Argument(name='sourceLevelCount', type='GLsizei'),
    ],
)

function(name='glCopyTextureSubImage1D', enabled=True, function_type=FuncType.COPY,
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

function(name='glCopyTextureSubImage1DEXT', enabled=True, function_type=FuncType.COPY,
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

function(name='glCopyTextureSubImage2D', enabled=True, function_type=FuncType.COPY,
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

function(name='glCopyTextureSubImage2DEXT', enabled=True, function_type=FuncType.COPY,
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

function(name='glCopyTextureSubImage3D', enabled=True, function_type=FuncType.COPY,
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

function(name='glCopyTextureSubImage3DEXT', enabled=True, function_type=FuncType.COPY,
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

function(name='glCoverFillPathInstancedNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glCoverFillPathNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='coverMode', type='GLenum'),
    ],
)

function(name='glCoverStrokePathInstancedNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glCoverStrokePathNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='coverMode', type='GLenum'),
    ],
)

function(name='glCoverageMaskNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLboolean'),
    ],
)

function(name='glCoverageModulationNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='components', type='GLenum'),
    ],
)

function(name='glCoverageModulationTableNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glCoverageOperationNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='operation', type='GLenum'),
    ],
)

function(name='glCreateBuffers', enabled=True, function_type=FuncType.CREATE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='buffers', type='GLuint*', wrap_type='CGLBuffer::CSMapArray', wrap_params='n, buffers'),
    ],
)

function(name='glCreateCommandListsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='lists', type='GLuint*'),
    ],
)

function(name='glCreateFramebuffers', enabled=True, function_type=FuncType.CREATE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='framebuffers', type='GLuint*', wrap_type='CGLFramebuffer::CSMapArray', wrap_params='n, framebuffers'),
    ],
)

function(name='glCreateMemoryObjectsEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='memoryObjects', type='GLuint*'),
    ],
)

function(name='glCreatePerfQueryINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='queryId', type='GLuint'),
        Argument(name='queryHandle', type='GLuint*'),
    ],
)

function(name='glCreateProgram', enabled=True, function_type=FuncType.CREATE, state_track=True,
    return_value=ReturnValue(type='GLuint', wrap_type='CGLProgram'),
    args=[],
)

function(name='glCreateProgramObjectARB', enabled=True, function_type=FuncType.CREATE, inherit_from='glCreateProgram',
    return_value=ReturnValue(type='GLhandleARB', wrap_type='CGLProgram'),
    args=[],
)

function(name='glCreateProgramPipelines', enabled=True, function_type=FuncType.CREATE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='pipelines', type='GLuint*', wrap_type='CGLPipeline::CSMapArray', wrap_params='n, pipelines'),
    ],
)

function(name='glCreateQueries', enabled=True, function_type=FuncType.GEN|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='n', type='GLsizei'),
        Argument(name='ids', type='GLuint*', wrap_type='CGLQuery::CSMapArray', wrap_params='n, ids'),
    ],
)

function(name='glCreateRenderbuffers', enabled=True, function_type=FuncType.CREATE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='renderbuffers', type='GLuint*', wrap_type='CGLRenderbuffer::CSMapArray', wrap_params='n, renderbuffers'),
    ],
)

function(name='glCreateSamplers', enabled=True, function_type=FuncType.CREATE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='samplers', type='GLuint*', wrap_type='CGLSampler::CSMapArray', wrap_params='n, samplers'),
    ],
)

function(name='glCreateSemaphoresNV', enabled=False, function_type=FuncType.CREATE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='semaphores', type='GLuint*'),
    ],
)

function(name='glCreateShader', enabled=True, function_type=FuncType.CREATE, state_track=True,
    return_value=ReturnValue(type='GLuint', wrap_type='CGLProgram'),
    args=[
        Argument(name='type', type='GLenum'),
    ],
)

function(name='glCreateShaderObjectARB', enabled=True, function_type=FuncType.CREATE, inherit_from='glCreateShader',
    return_value=ReturnValue(type='GLhandleARB', wrap_type='CGLProgram'),
    args=[
        Argument(name='shaderType', type='GLenum'),
    ],
)

function(name='glCreateShaderProgramEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLuint', wrap_type='CGLProgram'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='string', type='const GLchar*'),
    ],
)

function(name='glCreateShaderProgramv', enabled=True, function_type=FuncType.CREATE|FuncType.RESOURCE, state_track=True, run_wrap=True,
    return_value=ReturnValue(type='GLuint', wrap_type='CGLProgram'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='count', type='GLsizei', wrap_params='1'),
        Argument(name='strings', type='const GLchar*const*', wrap_type='CShaderSource', wrap_params='count, strings, _strings.SHADER_PROGRAM'),
    ],
)

function(name='glCreateShaderProgramvEXT', enabled=True, function_type=FuncType.CREATE|FuncType.RESOURCE, run_wrap=True, inherit_from='glCreateShaderProgramv')

function(name='glCreateStatesNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='states', type='GLuint*'),
    ],
)

function(name='glCreateSyncFromCLeventARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLsync'),
    args=[
        Argument(name='context', type='struct _cl_context*'),
        Argument(name='event', type='struct _cl_event*'),
        Argument(name='flags', type='GLbitfield'),
    ],
)

function(name='glCreateTextures', enabled=True, function_type=FuncType.CREATE, state_track=True, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='n', type='GLsizei'),
        Argument(name='textures', type='GLuint*', wrap_type='CGLTexture::CSMapArray', wrap_params='n, textures'),
    ],
)

function(name='glCreateTransformFeedbacks', enabled=True, function_type=FuncType.CREATE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='ids', type='GLuint*', wrap_type='CGLTransformFeedback::CSMapArray', wrap_params='n, ids'),
    ],
)

function(name='glCreateVertexArrays', enabled=True, function_type=FuncType.CREATE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='arrays', type='GLuint*', wrap_type='CGLVertexArray::CSMapArray', wrap_params='n, arrays'),
    ],
)

function(name='glCullFace', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glCullParameterdvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLdouble*'),
    ],
)

function(name='glCullParameterfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glCurrentPaletteMatrixARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLint'),
    ],
)

function(name='glCurrentPaletteMatrixOES', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='matrixpaletteindex', type='GLuint'),
    ],
)

function(name='glDebugMessageCallback', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='callback', type='GLDEBUGPROC'),
        Argument(name='userParam', type='const void*'),
    ],
)

function(name='glDebugMessageCallbackAMD', enabled=False, function_type=FuncType.NONE, inherit_from='glDebugMessageCallback',
    args=[
        Argument(name='callback', type='GLDEBUGPROCAMD'),
        Argument(name='userParam', type='void*'),
    ],
)

function(name='glDebugMessageCallbackARB', enabled=False, function_type=FuncType.NONE, inherit_from='glDebugMessageCallback',
    args=[
        Argument(name='callback', type='GLDEBUGPROCARB'),
        Argument(name='userParam', type='const void*'),
    ],
)

function(name='glDebugMessageCallbackKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glDebugMessageCallback',
    args=[
        Argument(name='callback',type='GLDEBUGPROCKHR'),
        Argument(name='userParam', type='const void*'),
    ],
)

function(name='glDebugMessageControl', enabled=False, function_type=FuncType.NONE,
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

function(name='glDebugMessageControlARB', enabled=False, function_type=FuncType.NONE, inherit_from='glDebugMessageControl')

function(name='glDebugMessageControlKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glDebugMessageControl')

function(name='glDebugMessageEnableAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='category', type='GLenum'),
        Argument(name='severity', type='GLenum'),
        Argument(name='count', type='GLsizei'),
        Argument(name='ids', type='const GLuint*'),
        Argument(name='enabled', type='GLboolean'),
    ],
)

function(name='glDebugMessageInsert', enabled=False, function_type=FuncType.NONE,
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

function(name='glDebugMessageInsertAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='category', type='GLenum'),
        Argument(name='severity', type='GLenum'),
        Argument(name='id', type='GLuint'),
        Argument(name='length', type='GLsizei'),
        Argument(name='buf', type='const GLchar*'),
    ],
)

function(name='glDebugMessageInsertARB', enabled=False, function_type=FuncType.NONE, inherit_from='glDebugMessageInsert')

function(name='glDebugMessageInsertKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glDebugMessageInsert')

function(name='glDeformSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLbitfield'),
    ],
)

function(name='glDeformationMap3dSGIX', enabled=False, function_type=FuncType.NONE,
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

function(name='glDeformationMap3fSGIX', enabled=False, function_type=FuncType.NONE,
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

function(name='glDeleteAsyncMarkersSGIX', enabled=True, function_type=FuncType.DELETE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='marker', type='GLuint'),
        Argument(name='range', type='GLsizei'),
    ],
)

function(name='glDeleteBuffers', enabled=True, function_type=FuncType.DELETE, state_track=True, run_condition='ConditionCurrentContextZero()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='buffers', type='const GLuint*', wrap_type='CGLBuffer::CSUnmapArray', wrap_params='n, buffers', remove_mapping=True),
    ],
)

function(name='glDeleteBuffersARB', enabled=True, function_type=FuncType.DELETE, inherit_from='glDeleteBuffers')

function(name='glDeleteCommandListsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='lists', type='const GLuint*'),
    ],
)

function(name='glDeleteFencesAPPLE', enabled=False, function_type=FuncType.DELETE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='fences', type='const GLuint*'),
    ],
)

function(name='glDeleteFencesNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='fences', type='const GLuint*'),
    ],
)

function(name='glDeleteFragmentShaderATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
    ],
)

function(name='glDeleteFramebuffers', enabled=True, function_type=FuncType.DELETE, state_track=True, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='framebuffers', type='const GLuint*', wrap_type='CGLFramebuffer::CSUnmapArray', wrap_params='n, framebuffers', remove_mapping=True),
    ],
)

function(name='glDeleteFramebuffersEXT', enabled=True, function_type=FuncType.DELETE, state_track=True, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='framebuffers', type='const GLuint*', wrap_type='CGLFramebufferEXT::CSUnmapArray', wrap_params='n, framebuffers', remove_mapping=True),
    ],
)

function(name='glDeleteFramebuffersOES', enabled=True, function_type=FuncType.DELETE, inherit_from='glDeleteFramebuffers')

function(name='glDeleteLists', enabled=True, function_type=FuncType.DELETE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='range', type='GLsizei'),
    ],
)

function(name='glDeleteMemoryObjectsEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='memoryObjects', type='const GLuint*'),
    ],
)

function(name='glDeleteNamedStringARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='namelen', type='GLint'),
        Argument(name='name', type='const GLchar*'),
    ],
)

function(name='glDeleteNamesAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='identifier', type='GLenum'),
        Argument(name='num', type='GLuint'),
        Argument(name='names', type='const GLuint*'),
    ],
)

function(name='glDeleteObjectARB', enabled=True, function_type=FuncType.DELETE, state_track=True, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='obj', type='GLhandleARB', wrap_type='CGLProgram'),
    ],
)

function(name='glDeleteOcclusionQueriesNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='ids', type='const GLuint*'),
    ],
)

function(name='glDeletePathsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='range', type='GLsizei'),
    ],
)

function(name='glDeletePerfMonitorsAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='monitors', type='GLuint*'),
    ],
)

function(name='glDeletePerfQueryINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='queryHandle', type='GLuint'),
    ],
)

function(name='glDeleteProgram', enabled=True, function_type=FuncType.RESOURCE, state_track=True, run_wrap=True, run_condition='ConditionCurrentContextZero()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

function(name='glDeleteProgramPipelines', enabled=True, function_type=FuncType.DELETE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='pipelines', type='const GLuint*', wrap_type='CGLPipeline::CSUnmapArray', wrap_params='n, pipelines', remove_mapping=True),
    ],
)

function(name='glDeleteProgramPipelinesEXT', enabled=True, function_type=FuncType.DELETE, inherit_from='glDeleteProgramPipelines')

function(name='glDeleteProgramsARB', enabled=True, function_type=FuncType.DELETE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='programs', type='const GLuint*', wrap_type='CGLProgram::CSUnmapArray', wrap_params='n, programs'),
    ],
)

function(name='glDeleteProgramsNV', enabled=True, function_type=FuncType.DELETE, inherit_from='glDeleteProgramsARB')

function(name='glDeleteQueries', enabled=True, function_type=FuncType.DELETE|FuncType.QUERY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='ids', type='const GLuint*', wrap_type='CGLQuery::CSUnmapArray', wrap_params='n, ids', remove_mapping=True),
    ],
)

function(name='glDeleteQueriesARB', enabled=True, function_type=FuncType.DELETE|FuncType.QUERY, inherit_from='glDeleteQueries')

function(name='glDeleteQueriesEXT', enabled=True, function_type=FuncType.DELETE|FuncType.QUERY, inherit_from='glDeleteQueries')

function(name='glDeleteQueryResourceTagNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='tagIds', type='const GLint*'),
    ],
)

function(name='glDeleteRenderbuffers', enabled=True, function_type=FuncType.DELETE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='renderbuffers', type='const GLuint*', wrap_type='CGLRenderbuffer::CSUnmapArray', wrap_params='n, renderbuffers', remove_mapping=True),
    ],
)

function(name='glDeleteRenderbuffersEXT', enabled=True, function_type=FuncType.DELETE, state_track=True, recorder_wrap=True, inherit_from='glDeleteRenderbuffers',
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='renderbuffers', type='const GLuint*', wrap_type='CGLRenderbufferEXT::CSUnmapArray', wrap_params='n, renderbuffers', remove_mapping=True),
    ],
)

function(name='glDeleteRenderbuffersOES', enabled=True, function_type=FuncType.DELETE, inherit_from='glDeleteRenderbuffers')

function(name='glDeleteSamplers', enabled=True, function_type=FuncType.DELETE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='count', type='GLsizei'),
        Argument(name='samplers', type='const GLuint*', wrap_type='CGLSampler::CSUnmapArray', wrap_params='count, samplers', remove_mapping=True),
    ],
)

function(name='glDeleteSemaphoresEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='semaphores', type='const GLuint*'),
    ],
)

function(name='glDeleteShader', enabled=True, function_type=FuncType.DELETE, state_track=True, recorder_wrap=True, run_condition='ConditionCurrentContextZero()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
    ],
)

function(name='glDeleteStatesNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='states', type='const GLuint*'),
    ],
)

function(name='glDeleteSync', enabled=True, function_type=FuncType.DELETE, run_condition='ConditionCurrentContextZero()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sync', type='GLsync', wrap_type='CGLsync', remove_mapping=True),
    ],
)

function(name='glDeleteSyncAPPLE', enabled=False, function_type=FuncType.DELETE, inherit_from='glDeleteSync')

function(name='glDeleteTextures', enabled=True, function_type=FuncType.DELETE, state_track=True, run_condition='ConditionCurrentContextZero()', rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='textures', type='const GLuint*', wrap_type='CGLTexture::CSUnmapArray', wrap_params='n, textures', remove_mapping=True),
    ],
)

function(name='glDeleteTexturesEXT', enabled=True, function_type=FuncType.DELETE, inherit_from='glDeleteTextures', run_condition=False)

function(name='glDeleteTransformFeedbacks', enabled=True, function_type=FuncType.DELETE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='ids', type='const GLuint*',wrap_type='CGLTransformFeedback::CSUnmapArray', wrap_params='n, ids', remove_mapping=True),
    ],
)

function(name='glDeleteTransformFeedbacksNV', enabled=True, function_type=FuncType.DELETE, inherit_from='glDeleteTransformFeedbacks')

function(name='glDeleteVertexArrays', enabled=True, function_type=FuncType.DELETE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='arrays', type='const GLuint*', wrap_type='CGLVertexArray::CSUnmapArray', wrap_params='n, arrays', remove_mapping=True),
    ],
)

function(name='glDeleteVertexArraysAPPLE', enabled=False, function_type=FuncType.DELETE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='arrays', type='const GLuint*'),
    ],
)

function(name='glDeleteVertexArraysOES', enabled=True, function_type=FuncType.DELETE, inherit_from='glDeleteVertexArrays')

function(name='glDeleteVertexShaderEXT', enabled=True, function_type=FuncType.DELETE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
    ],
)

function(name='glDepthBoundsEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='zmin', type='GLclampd'),
        Argument(name='zmax', type='GLclampd'),
    ],
)

function(name='glDepthBoundsdNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='zmin', type='GLdouble'),
        Argument(name='zmax', type='GLdouble'),
    ],
)

function(name='glDepthFunc', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='func', type='GLenum'),
    ],
)

function(name='glDepthMask', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='flag', type='GLboolean'),
    ],
)

function(name='glDepthRange', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLdouble'),
        Argument(name='f', type='GLdouble'),
    ],
)

function(name='glDepthRangeArrayfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glDepthRangeArrayfvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glDepthRangeArrayv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLdouble*', wrap_params='count, v'),
    ],
)

function(name='glDepthRangeIndexed', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='n', type='GLdouble'),
        Argument(name='f', type='GLdouble'),
    ],
)

function(name='glDepthRangeIndexedfNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='n', type='GLfloat'),
        Argument(name='f', type='GLfloat'),
    ],
)

function(name='glDepthRangeIndexedfOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='n', type='GLfloat'),
        Argument(name='f', type='GLfloat'),
    ],
)

function(name='glDepthRangedNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='zNear', type='GLdouble'),
        Argument(name='zFar', type='GLdouble'),
    ],
)

function(name='glDepthRangef', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLfloat'),
        Argument(name='f', type='GLfloat'),
    ],
)

function(name='glDepthRangefOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glDepthRangef',
    args=[
        Argument(name='zNear', type='GLclampf'),
        Argument(name='zFar', type='GLclampf'),
    ],
)

function(name='glDepthRangex', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLfixed'),
        Argument(name='f', type='GLfixed'),
    ],
)

function(name='glDepthRangexOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glDepthRangex')

function(name='glDetachObjectARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='containerObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='attachedObj', type='GLhandleARB', wrap_type='CGLProgram'),
    ],
)

function(name='glDetachShader', enabled=True, function_type=FuncType.PARAM, run_condition='ConditionCurrentContextZero()', state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
    ],
)

function(name='glDetailTexFuncSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='n', type='GLsizei'),
        Argument(name='points', type='const GLfloat*'),
    ],
)

function(name='glDisable', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='cap', type='GLenum'),
    ],
)

function(name='glDisableClientState', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLenum'),
    ],
)

function(name='glDisableClientStateIndexedEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

function(name='glDisableClientStateiEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

function(name='glDisableDriverControlQCOM', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='driverControl', type='GLuint'),
    ],
)

function(name='glDisableIndexedEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

function(name='glDisableVariantClientStateEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
    ],
)

function(name='glDisableVertexArrayAttrib', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='index', type='GLuint'),
    ],
)

function(name='glDisableVertexArrayAttribEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glDisableVertexArrayAttrib')

function(name='glDisableVertexArrayEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='array', type='GLenum'),
    ],
)

function(name='glDisableVertexAttribAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
    ],
)

function(name='glDisableVertexAttribArray', enabled=True, function_type=FuncType.PARAM, state_track=True, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
    ],
)

function(name='glDisableVertexAttribArrayARB', enabled=True, function_type=FuncType.PARAM, recorder_wrap=True, inherit_from='glDisableVertexAttribArray')

function(name='glDisablei', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

function(name='glDisableiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glDisablei')

function(name='glDisableiNV', enabled=False, function_type=FuncType.NONE, inherit_from='glDisablei')

function(name='glDisableiOES', enabled=False, function_type=FuncType.NONE, inherit_from='glDisablei')

function(name='glDiscardFramebufferEXT', enabled=True, function_type=FuncType.PARAM, run_condition='ConditionExtension("GL_EXT_discard_framebuffer")',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='numAttachments', type='GLsizei'),
        Argument(name='attachments', type='const GLenum*', wrap_params='numAttachments, attachments'),
    ],
)

function(name='glDispatchCompute', enabled=True, function_type=FuncType.RENDER,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='num_groups_x', type='GLuint'),
        Argument(name='num_groups_y', type='GLuint'),
        Argument(name='num_groups_z', type='GLuint'),
    ],
)

function(name='glDispatchComputeGroupSizeARB', enabled=False, function_type=FuncType.NONE,
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

function(name='glDispatchComputeIndirect', enabled=True, function_type=FuncType.RENDER,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='indirect', type='GLintptr'),
    ],
)

function(name='glDrawArrays', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(first, count, 0, 0)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='first', type='GLint'),
        Argument(name='count', type='GLsizei'),
    ],
)

function(name='glDrawArraysEXT', enabled=True, function_type=FuncType.RENDER, inherit_from='glDrawArrays')

function(name='glDrawArraysIndirect', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientIndirectArraysUpdate(mode, indirect, 1, 0)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='indirect', type='const void*', wrap_type='CIndirectPtr'),
    ],
)

function(name='glDrawArraysInstanced', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(first, count, instancecount, 0)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='first', type='GLint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='instancecount', type='GLsizei'),
    ],
)

function(name='glDrawArraysInstancedANGLE', enabled=True, function_type=FuncType.RENDER, exec_post_recorder_wrap=True, inherit_from='glDrawArraysInstanced')

function(name='glDrawArraysInstancedARB', enabled=True, function_type=FuncType.RENDER, inherit_from='glDrawArraysInstanced')

function(name='glDrawArraysInstancedBaseInstance', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(first, count, instancecount, baseinstance)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='first', type='GLint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='instancecount', type='GLsizei'),
        Argument(name='baseinstance', type='GLuint'),
    ],
)

function(name='glDrawArraysInstancedBaseInstanceEXT', enabled=False, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(first, count, instancecount, baseinstance)', inherit_from='glDrawArraysInstancedBaseInstance')

function(name='glDrawArraysInstancedEXT', enabled=True, function_type=FuncType.RENDER, inherit_from='glDrawArraysInstanced')

function(name='glDrawArraysInstancedNV', enabled=False, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(first, count, instancecount, 0)', inherit_from='glDrawArraysInstanced')

function(name='glDrawBuffer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buf', type='GLenum'),
    ],
)

function(name='glDrawBuffers', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='bufs', type='const GLenum*', wrap_params='n,bufs'),
    ],
)

function(name='glDrawBuffersARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glDrawBuffers')

function(name='glDrawBuffersATI', enabled=True, function_type=FuncType.PARAM, inherit_from='glDrawBuffers')

function(name='glDrawBuffersEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glDrawBuffers')

function(name='glDrawBuffersIndexedEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLint'),
        Argument(name='location', type='const GLenum*'),
        Argument(name='indices', type='const GLint*'),
    ],
)

function(name='glDrawBuffersNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glDrawBuffers')

function(name='glDrawCommandsAddressNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='primitiveMode', type='GLenum'),
        Argument(name='indirects', type='const GLuint64*'),
        Argument(name='sizes', type='const GLsizei*'),
        Argument(name='count', type='GLuint'),
    ],
)

function(name='glDrawCommandsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='primitiveMode', type='GLenum'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='indirects', type='const GLintptr*'),
        Argument(name='sizes', type='const GLsizei*'),
        Argument(name='count', type='GLuint'),
    ],
)

function(name='glDrawCommandsStatesAddressNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='indirects', type='const GLuint64*'),
        Argument(name='sizes', type='const GLsizei*'),
        Argument(name='states', type='const GLuint*'),
        Argument(name='fbos', type='const GLuint*'),
        Argument(name='count', type='GLuint'),
    ],
)

function(name='glDrawCommandsStatesNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glDrawElementArrayAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='first', type='GLint'),
        Argument(name='count', type='GLsizei'),
    ],
)

function(name='glDrawElementArrayATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='count', type='GLsizei'),
    ],
)

function(name='glDrawElements', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, 0, 0, 0)', rec_condition='count>=0 && Recording(_recorder)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='count', type='GLsizei'),
        Argument(name='type', type='GLenum'),
        Argument(name='indices', type='const void*', wrap_type='CIndexPtr'),
    ],
)

function(name='glDrawElementsBaseVertex', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, 0, 0, basevertex)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='count', type='GLsizei'),
        Argument(name='type', type='GLenum'),
        Argument(name='indices', type='const void*', wrap_type='CIndexPtr'),
        Argument(name='basevertex', type='GLint'),
    ],
)

function(name='glDrawElementsBaseVertexEXT', enabled=False, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, 0, 0, basevertex)', inherit_from='glDrawElementsBaseVertex')

function(name='glDrawElementsBaseVertexOES', enabled=False, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, 0, 0, basevertex)', inherit_from='glDrawElementsBaseVertex')

function(name='glDrawElementsIndirect', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientIndirectArraysUpdate(mode, type, indirect, 1, 0)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='indirect', type='const void*', wrap_type='CIndirectPtr'),
    ],
)

function(name='glDrawElementsInstanced', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, instancecount, 0, 0)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='count', type='GLsizei'),
        Argument(name='type', type='GLenum'),
        Argument(name='indices', type='const void*', wrap_type='CIndexPtr'),
        Argument(name='instancecount', type='GLsizei'),
    ],
)

function(name='glDrawElementsInstancedANGLE', enabled=True, function_type=FuncType.RENDER, inherit_from='glDrawElementsInstanced')

function(name='glDrawElementsInstancedARB', enabled=True, function_type=FuncType.RENDER, inherit_from='glDrawElementsInstanced')

function(name='glDrawElementsInstancedBaseInstance', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, instancecount, baseinstance, 0)',
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

function(name='glDrawElementsInstancedBaseInstanceEXT', enabled=False, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, instancecount, baseinstance, 0)', inherit_from='glDrawElementsInstancedBaseInstance')

function(name='glDrawElementsInstancedBaseVertex', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, instancecount, 0, basevertex)', exec_post_recorder_wrap=True,
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

function(name='glDrawElementsInstancedBaseVertexBaseInstance', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, instancecount, baseinstance, basevertex)',
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

function(name='glDrawElementsInstancedBaseVertexBaseInstanceEXT', enabled=False, function_type=FuncType.RENDER, inherit_from='glDrawElementsInstancedBaseVertexBaseInstance')

function(name='glDrawElementsInstancedBaseVertexEXT', enabled=False, function_type=FuncType.RENDER, inherit_from='glDrawElementsInstancedBaseVertex')

function(name='glDrawElementsInstancedBaseVertexOES', enabled=False, function_type=FuncType.RENDER, inherit_from='glDrawElementsInstancedBaseVertex')

function(name='glDrawElementsInstancedEXT', enabled=True, function_type=FuncType.RENDER, inherit_from='glDrawElementsInstanced')

function(name='glDrawElementsInstancedNV', enabled=False, function_type=FuncType.RENDER, inherit_from='glDrawElementsInstanced')

function(name='glDrawMeshArraysSUN', enabled=False, function_type=FuncType.NONE, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='first', type='GLint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='width', type='GLsizei'),
    ],
)

function(name='glDrawPixels', enabled=True, function_type=FuncType.FILL, pre_schedule='coherentBufferUpdate_PS(_recorder)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='const void*', wrap_type='CGLTexResource', wrap_params='GL_TEXTURE_2D, format, type, width, height, pixels'),
    ],
)

function(name='glDrawRangeElementArrayAPPLE', enabled=False, function_type=FuncType.NONE, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='start', type='GLuint'),
        Argument(name='end', type='GLuint'),
        Argument(name='first', type='GLint'),
        Argument(name='count', type='GLsizei'),
    ],
)

function(name='glDrawRangeElementArrayATI', enabled=False, function_type=FuncType.NONE, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='start', type='GLuint'),
        Argument(name='end', type='GLuint'),
        Argument(name='count', type='GLsizei'),
    ],
)

function(name='glDrawRangeElements', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, 0, 0, 0)', exec_post_recorder_wrap=True,
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

function(name='glDrawRangeElementsBaseVertex', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(count, type, indices, 0, 0, basevertex)', exec_post_recorder_wrap=True,
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

function(name='glDrawRangeElementsBaseVertexEXT', enabled=False, function_type=FuncType.RENDER, inherit_from='glDrawRangeElementsBaseVertex')

function(name='glDrawRangeElementsBaseVertexOES', enabled=False, function_type=FuncType.RENDER, inherit_from='glDrawRangeElementsBaseVertex')

function(name='glDrawRangeElementsEXT', enabled=True, function_type=FuncType.RENDER, inherit_from='glDrawRangeElements')

function(name='glDrawTexfOES', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='width', type='GLfloat'),
        Argument(name='height', type='GLfloat'),
    ],
)

function(name='glDrawTexfvOES', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfloat*', wrap_params='5, coords'),
    ],
)

function(name='glDrawTexiOES', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
        Argument(name='width', type='GLint'),
        Argument(name='height', type='GLint'),
    ],
)

function(name='glDrawTexivOES', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLint*', wrap_params='5, coords'),
    ],
)

function(name='glDrawTexsOES', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
        Argument(name='width', type='GLshort'),
        Argument(name='height', type='GLshort'),
    ],
)

function(name='glDrawTexsvOES', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLshort*', wrap_params='5, coords'),
    ],
)

function(name='glDrawTextureNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glDrawTexxOES', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfixed'),
        Argument(name='y', type='GLfixed'),
        Argument(name='z', type='GLfixed'),
        Argument(name='width', type='GLfixed'),
        Argument(name='height', type='GLfixed'),
    ],
)

function(name='glDrawTexxvOES', enabled=True, function_type=FuncType.FILL, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*', wrap_params='5, coords'),
    ],
)

function(name='glDrawTransformFeedback', enabled=True, function_type=FuncType.RENDER,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='id', type='GLuint'),
    ],
)

function(name='glDrawTransformFeedbackEXT', enabled=False, function_type=FuncType.RENDER, inherit_from='glDrawTransformFeedback')

function(name='glDrawTransformFeedbackInstanced', enabled=True, function_type=FuncType.RENDER,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='id', type='GLuint'),
        Argument(name='instancecount', type='GLsizei'),
    ],
)

function(name='glDrawTransformFeedbackInstancedEXT', enabled=False, function_type=FuncType.RENDER, inherit_from='glDrawTransformFeedbackInstanced')

function(name='glDrawTransformFeedbackNV', enabled=True, function_type=FuncType.RENDER, inherit_from='glDrawTransformFeedback')

function(name='glDrawTransformFeedbackStream', enabled=True, function_type=FuncType.RENDER,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='id', type='GLuint'),
        Argument(name='stream', type='GLuint'),
    ],
)

function(name='glDrawTransformFeedbackStreamInstanced', enabled=True, function_type=FuncType.RENDER,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='id', type='GLuint'),
        Argument(name='stream', type='GLuint'),
        Argument(name='instancecount', type='GLsizei'),
    ],
)

function(name='glDrawVkImageNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glEGLImageTargetRenderbufferStorageOES', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='image', type='GLeglImageOES', wrap_type='CEGLImageKHR'),
    ],
)

function(name='glEGLImageTargetTexStorageEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='image', type='GLeglImageOES', wrap_type='CEGLImageKHR'),
        Argument(name='attrib_list', type='const GLint*'),
    ],
)

function(name='glEGLImageTargetTexture2DOES', enabled=True, function_type=FuncType.PARAM, run_wrap=True, state_track=True, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='image', type='GLeglImageOES', wrap_type='CEGLImageKHR'),
    ],
)

function(name='glEGLImageTargetTextureStorageEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='image', type='GLeglImageOES', wrap_type='CEGLImageKHR'),
        Argument(name='attrib_list', type='const GLint*'),
    ],
)

function(name='glEdgeFlag', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='flag', type='GLboolean'),
    ],
)

function(name='glEdgeFlagFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stride', type='GLsizei'),
    ],
)

function(name='glEdgeFlagPointer', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glEdgeFlagPointerEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stride', type='GLsizei'),
        Argument(name='count', type='GLsizei'),
        Argument(name='pointer', type='const GLboolean*'),
    ],
)

function(name='glEdgeFlagPointerListIBM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stride', type='GLint'),
        Argument(name='pointer', type='const GLboolean**'),
        Argument(name='ptrstride', type='GLint'),
    ],
)

function(name='glEdgeFlagv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='flag', type='const GLboolean*'),
    ],
)

function(name='glElementPointerAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glElementPointerATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glEnable', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='cap', type='GLenum'),
    ],
)

function(name='glEnableClientState', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLenum'),
    ],
)

function(name='glEnableClientStateIndexedEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

function(name='glEnableClientStateiEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

function(name='glEnableDriverControlQCOM', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='driverControl', type='GLuint'),
    ],
)

function(name='glEnableIndexedEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

function(name='glEnableVariantClientStateEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
    ],
)

function(name='glEnableVertexArrayAttrib', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='index', type='GLuint'),
    ],
)

function(name='glEnableVertexArrayAttribEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glEnableVertexArrayAttrib')

function(name='glEnableVertexArrayEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='array', type='GLenum'),
    ],
)

function(name='glEnableVertexAttribAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
    ],
)

function(name='glEnableVertexAttribArray', enabled=True, function_type=FuncType.PARAM, recorder_wrap=True, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
    ],
)

function(name='glEnableVertexAttribArrayARB', enabled=True, function_type=FuncType.PARAM, recorder_wrap=True, inherit_from='glEnableVertexAttribArray')

function(name='glEnablei', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

function(name='glEnableiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glEnablei')

function(name='glEnableiNV', enabled=False, function_type=FuncType.NONE, inherit_from='glEnablei')

function(name='glEnableiOES', enabled=False, function_type=FuncType.NONE, inherit_from='glEnablei')

function(name='glEnd', enabled=True, function_type=FuncType.PARAM, run_wrap=True, state_track=True, suffix="""// See comment in glBegin for rationale of following line.
  globalMutex.unlock();""",
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glEndConditionalRender', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glEndConditionalRenderNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glEndConditionalRender')

function(name='glEndConditionalRenderNVX', enabled=False, function_type=FuncType.NONE, inherit_from='glEndConditionalRender')

function(name='glEndFragmentShaderATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glEndList', enabled=True, function_type=FuncType.PARAM, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glEndOcclusionQueryNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glEndPerfMonitorAMD', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='monitor', type='GLuint'),
    ],
)

function(name='glEndPerfQueryINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='queryHandle', type='GLuint'),
    ],
)

function(name='glEndQuery', enabled=True, function_type=FuncType.PARAM|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

function(name='glEndQueryARB', enabled=True, function_type=FuncType.PARAM|FuncType.QUERY, inherit_from='glEndQuery')

function(name='glEndQueryEXT', enabled=True, function_type=FuncType.PARAM|FuncType.QUERY, inherit_from='glEndQuery')

function(name='glEndQueryIndexed', enabled=True, function_type=FuncType.PARAM|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

function(name='glEndTilingQCOM', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='preserveMask', type='GLbitfield'),
    ],
)

function(name='glEndTransformFeedback', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glEndTransformFeedbackEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glEndTransformFeedback')

function(name='glEndTransformFeedbackNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glEndTransformFeedback')

function(name='glEndVertexShaderEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glEndVideoCaptureNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
    ],
)

function(name='glEvalCoord1d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='GLdouble'),
    ],
)

function(name='glEvalCoord1dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='const GLdouble*', wrap_params='1, u'),
    ],
)

function(name='glEvalCoord1f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='GLfloat'),
    ],
)

function(name='glEvalCoord1fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='const GLfloat*', wrap_params='1, u'),
    ],
)

function(name='glEvalCoord1xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='GLfixed'),
    ],
)

function(name='glEvalCoord1xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

function(name='glEvalCoord2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='GLdouble'),
        Argument(name='v', type='GLdouble'),
    ],
)

function(name='glEvalCoord2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='const GLdouble*', wrap_params='2, u'),
    ],
)

function(name='glEvalCoord2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='GLfloat'),
        Argument(name='v', type='GLfloat'),
    ],
)

function(name='glEvalCoord2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='const GLfloat*', wrap_params='2, u'),
    ],
)

function(name='glEvalCoord2xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='u', type='GLfixed'),
        Argument(name='v', type='GLfixed'),
    ],
)

function(name='glEvalCoord2xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

function(name='glEvalMapsNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glEvalMesh1', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='i1', type='GLint'),
        Argument(name='i2', type='GLint'),
    ],
)

function(name='glEvalMesh2', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='i1', type='GLint'),
        Argument(name='i2', type='GLint'),
        Argument(name='j1', type='GLint'),
        Argument(name='j2', type='GLint'),
    ],
)

function(name='glEvalPoint1', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='i', type='GLint'),
    ],
)

function(name='glEvalPoint2', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='i', type='GLint'),
        Argument(name='j', type='GLint'),
    ],
)

function(name='glEvaluateDepthValuesARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glExecuteProgramNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='id', type='GLuint'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

function(name='glExtGetBufferPointervQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='params', type='void**'),
    ],
)

function(name='glExtGetBuffersQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffers', type='GLuint*'),
        Argument(name='maxBuffers', type='GLint'),
        Argument(name='numBuffers', type='GLint*'),
    ],
)

function(name='glExtGetFramebuffersQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffers', type='GLuint*'),
        Argument(name='maxFramebuffers', type='GLint'),
        Argument(name='numFramebuffers', type='GLint*'),
    ],
)

function(name='glExtGetProgramBinarySourceQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='shadertype', type='GLenum'),
        Argument(name='source', type='GLchar*'),
        Argument(name='length', type='GLint*'),
    ],
)

function(name='glExtGetProgramsQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='programs', type='GLuint*'),
        Argument(name='maxPrograms', type='GLint'),
        Argument(name='numPrograms', type='GLint*'),
    ],
)

function(name='glExtGetRenderbuffersQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='renderbuffers', type='GLuint*'),
        Argument(name='maxRenderbuffers', type='GLint'),
        Argument(name='numRenderbuffers', type='GLint*'),
    ],
)

function(name='glExtGetShadersQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shaders', type='GLuint*'),
        Argument(name='maxShaders', type='GLint'),
        Argument(name='numShaders', type='GLint*'),
    ],
)

function(name='glExtGetTexLevelParameterivQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='face', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glExtGetTexSubImageQCOM', enabled=False, function_type=FuncType.NONE,
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

function(name='glExtGetTexturesQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='textures', type='GLuint*'),
        Argument(name='maxTextures', type='GLint'),
        Argument(name='numTextures', type='GLint*'),
    ],
)

function(name='glExtIsProgramBinaryQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

function(name='glExtTexObjectStateOverrideiQCOM', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glExtractComponentEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='res', type='GLuint'),
        Argument(name='src', type='GLuint'),
        Argument(name='num', type='GLuint'),
    ],
)

function(name='glFeedbackBuffer', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLsizei'),
        Argument(name='type', type='GLenum'),
        Argument(name='buffer', type='GLfloat*'),
    ],
)

function(name='glFeedbackBufferxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='type', type='GLenum'),
        Argument(name='buffer', type='const GLfixed*'),
    ],
)

function(name='glFenceSync', enabled=True, function_type=FuncType.CREATE,
    return_value=ReturnValue(type='GLsync', wrap_type='CGLsync'),
    args=[
        Argument(name='condition', type='GLenum'),
        Argument(name='flags', type='GLbitfield'),
    ],
)

function(name='glFenceSyncAPPLE', enabled=False, function_type=FuncType.CREATE, inherit_from='glFenceSync')

function(name='glFinalCombinerInputNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='variable', type='GLenum'),
        Argument(name='input', type='GLenum'),
        Argument(name='mapping', type='GLenum'),
        Argument(name='componentUsage', type='GLenum'),
    ],
)

function(name='glFinish', enabled=True, function_type=FuncType.EXEC, run_wrap=True, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glFinishAsyncSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='markerp', type='GLuint*'),
    ],
)

function(name='glFinishFenceAPPLE', enabled=False, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='fence', type='GLuint'),
    ],
)

function(name='glFinishFenceNV', enabled=True, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='fence', type='GLuint'),
    ],
)

function(name='glFinishObjectAPPLE', enabled=False, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='object', type='GLenum'),
        Argument(name='name', type='GLint'),
    ],
)

function(name='glFinishRenderAPPLE', enabled=False, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glFinishTextureSUNX', enabled=True, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glFlush', enabled=True, function_type=FuncType.EXEC, run_wrap=True, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glFlushMappedBufferRange', enabled=True, function_type=FuncType.EXEC, state_track=True, pre_token='CgitsFlushMappedBufferRange(target,offset,length)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='length', type='GLsizeiptr'),
    ],
)

function(name='glFlushMappedBufferRangeAPPLE', enabled=False, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
    ],
)

function(name='glFlushMappedBufferRangeEXT', enabled=True, function_type=FuncType.EXEC, inherit_from='glFlushMappedBufferRange', pre_token=False )

function(name='glFlushMappedNamedBufferRange', enabled=True, function_type=FuncType.EXEC, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='length', type='GLsizeiptr'),
    ],
)

function(name='glFlushMappedNamedBufferRangeEXT', enabled=True, function_type=FuncType.EXEC, inherit_from='glFlushMappedNamedBufferRange')

function(name='glFlushPixelDataRangeNV', enabled=True, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

function(name='glFlushRasterSGIX', enabled=True, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glFlushRenderAPPLE', enabled=False, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glFlushStaticDataIBM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

function(name='glFlushVertexArrayRangeAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='length', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glFlushVertexArrayRangeNV', enabled=True, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glFogCoordFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
    ],
)

function(name='glFogCoordPointer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

function(name='glFogCoordPointerEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glFogCoordPointer')

function(name='glFogCoordPointerListIBM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLint'),
        Argument(name='pointer', type='const void**'),
        Argument(name='ptrstride', type='GLint'),
    ],
)

function(name='glFogCoordd', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLdouble'),
    ],
)

function(name='glFogCoorddEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glFogCoordd')

function(name='glFogCoorddv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='const GLdouble*', wrap_params='1, coord'),
    ],
)

function(name='glFogCoorddvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glFogCoorddv')

function(name='glFogCoordf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLfloat'),
    ],
)

function(name='glFogCoordfEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glFogCoordf')

function(name='glFogCoordfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='const GLfloat*', wrap_params='1, coord'),
    ],
)

function(name='glFogCoordfvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glFogCoordfv')

function(name='glFogCoordhNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='fog', type='GLhalfNV'),
    ],
)

function(name='glFogCoordhvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='fog', type='const GLhalfNV*', wrap_params='1, fog'),
    ],
)

function(name='glFogFuncSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='points', type='const GLfloat*'),
    ],
)

function(name='glFogf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glFogfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glFogi', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glFogiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_params='1, params'),
    ],
)

function(name='glFogx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

function(name='glFogxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glFogx')

function(name='glFogxv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfixed*', wrap_params='1, params'),
    ],
)

function(name='glFogxvOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glFogxv')

function(name='glFragmentColorMaterialSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glFragmentCoverageColorNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='color', type='GLuint'),
    ],
)

function(name='glFragmentLightModelfSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glFragmentLightModelfvSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

function(name='glFragmentLightModeliSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glFragmentLightModelivSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glFragmentLightfSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glFragmentLightfvSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

function(name='glFragmentLightiSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glFragmentLightivSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glFragmentMaterialfSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glFragmentMaterialfvSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

function(name='glFragmentMaterialiSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glFragmentMaterialivSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glFrameTerminatorGREMEDY', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glFrameZoomSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLint'),
    ],
)

function(name='glFramebufferDrawBufferEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebufferEXT'),
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glFramebufferDrawBuffersEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='n', type='GLsizei'),
        Argument(name='bufs', type='const GLenum*'),
    ],
)

function(name='glFramebufferFetchBarrierEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glFramebufferFetchBarrierQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glFramebufferFoveationConfigQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='numLayers', type='GLuint'),
        Argument(name='focalPointsPerLayer', type='GLuint'),
        Argument(name='requestedFeatures', type='GLuint'),
        Argument(name='providedFeatures', type='GLuint*'),
    ],
)

function(name='glFramebufferFoveationParametersQCOM', enabled=False, function_type=FuncType.NONE,
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

function(name='glFramebufferParameteri', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glFramebufferPixelLocalStorageSizeEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLuint'),
        Argument(name='size', type='GLsizei'),
    ],
)

function(name='glFramebufferReadBufferEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebufferEXT'),
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glFramebufferRenderbuffer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='renderbuffertarget', type='GLenum'),
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
    ],
)

function(name='glFramebufferRenderbufferEXT', enabled=True, function_type=FuncType.PARAM, state_track=True, recorder_wrap=True, inherit_from='glFramebufferRenderbuffer',
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='renderbuffertarget', type='GLenum'),
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbufferEXT'),
    ],
)

function(name='glFramebufferRenderbufferOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glFramebufferRenderbuffer')

function(name='glFramebufferSampleLocationsfvARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='start', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glFramebufferSampleLocationsfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='start', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glFramebufferSamplePositionsfvAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='numsamples', type='GLuint'),
        Argument(name='pixelindex', type='GLuint'),
        Argument(name='values', type='const GLfloat*'),
    ],
)

function(name='glFramebufferTexture', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
    ],
)

function(name='glFramebufferTexture1D', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='textarget', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
    ],
)

function(name='glFramebufferTexture1DEXT', enabled=True, function_type=FuncType.PARAM, state_track=True, recorder_wrap=True, inherit_from='glFramebufferTexture1D')

function(name='glFramebufferTexture2D', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='textarget', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
    ],
)

function(name='glFramebufferTexture2DDownsampleIMG', enabled=False, function_type=FuncType.NONE,
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

function(name='glFramebufferTexture2DEXT', enabled=True, function_type=FuncType.PARAM, state_track=True, recorder_wrap=True, inherit_from='glFramebufferTexture2D')

function(name='glFramebufferTexture2DMultisampleEXT', enabled=True, function_type=FuncType.PARAM,
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

function(name='glFramebufferTexture2DMultisampleIMG', enabled=True, function_type=FuncType.PARAM,
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

function(name='glFramebufferTexture2DOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glFramebufferTexture2D')

function(name='glFramebufferTexture3D', enabled=True, function_type=FuncType.PARAM, state_track=True,
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

function(name='glFramebufferTexture3DEXT', enabled=True, function_type=FuncType.PARAM, state_track=True, recorder_wrap=True, inherit_from='glFramebufferTexture3D')

function(name='glFramebufferTexture3DOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glFramebufferTexture3D')

function(name='glFramebufferTextureARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glFramebufferTexture')

function(name='glFramebufferTextureEXT', enabled=True, function_type=FuncType.PARAM, state_track=True, recorder_wrap=True, inherit_from='glFramebufferTexture')

function(name='glFramebufferTextureFaceARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='face', type='GLenum'),
    ],
)

function(name='glFramebufferTextureFaceEXT', enabled=True, function_type=FuncType.PARAM, state_track=True, recorder_wrap=True, inherit_from='glFramebufferTextureFaceARB')

function(name='glFramebufferTextureLayer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='layer', type='GLint'),
    ],
)

function(name='glFramebufferTextureLayerARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glFramebufferTextureLayer')

function(name='glFramebufferTextureLayerDownsampleIMG', enabled=False, function_type=FuncType.NONE,
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

function(name='glFramebufferTextureLayerEXT', enabled=True, function_type=FuncType.PARAM, state_track=True, recorder_wrap=True, inherit_from='glFramebufferTextureLayer')

function(name='glFramebufferTextureMultisampleMultiviewOVR', enabled=False, function_type=FuncType.NONE,
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

function(name='glFramebufferTextureMultiviewOVR', enabled=False, function_type=FuncType.NONE,
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

function(name='glFramebufferTextureOES', enabled=False, function_type=FuncType.NONE, inherit_from='glFramebufferTexture')

function(name='glFreeObjectBufferATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

function(name='glFrontFace', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glFrustum', enabled=True, function_type=FuncType.PARAM,
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

function(name='glFrustumf', enabled=True, function_type=FuncType.PARAM,
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

function(name='glFrustumfOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glFrustumf')

function(name='glFrustumx', enabled=True, function_type=FuncType.PARAM,
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

function(name='glFrustumxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glFrustumx')

function(name='glGenAsyncMarkersSGIX', enabled=True, function_type=FuncType.GEN,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='range', type='GLsizei'),
    ],
)

function(name='glGenBuffers', enabled=True, function_type=FuncType.GEN, state_track=True, rec_condition='ConditionBufferES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='buffers', type='GLuint*', wrap_type='CGLBuffer::CSMapArray', wrap_params='n, buffers'),
    ],
)

function(name='glGenBuffersARB', enabled=True, function_type=FuncType.GEN, inherit_from='glGenBuffers')

function(name='glGenFencesAPPLE', enabled=False, function_type=FuncType.GEN,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='fences', type='GLuint*'),
    ],
)

function(name='glGenFencesNV', enabled=True, function_type=FuncType.GEN,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='fences', type='GLuint*', wrap_type='CGLfence::CSMapArray', wrap_params='n, fences'),
    ],
)

function(name='glGenFragmentShadersATI', enabled=True, function_type=FuncType.GEN,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='range', type='GLuint'),
    ],
)

function(name='glGenFramebuffers', enabled=True, function_type=FuncType.GEN, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='framebuffers', type='GLuint*', wrap_type='CGLFramebuffer::CSMapArray', wrap_params='n, framebuffers'),
    ],
)

function(name='glGenFramebuffersEXT', enabled=True, function_type=FuncType.GEN, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='framebuffers', type='GLuint*', wrap_type='CGLFramebufferEXT::CSMapArray', wrap_params='n, framebuffers'),
    ],
)

function(name='glGenFramebuffersOES', enabled=True, function_type=FuncType.GEN, recorder_wrap=False, inherit_from='glGenFramebuffersEXT')

function(name='glGenLists', enabled=True, function_type=FuncType.GEN,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='range', type='GLsizei'),
    ],
)

function(name='glGenNamesAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='identifier', type='GLenum'),
        Argument(name='num', type='GLuint'),
        Argument(name='names', type='GLuint*'),
    ],
)

function(name='glGenOcclusionQueriesNV', enabled=False, function_type=FuncType.GEN|FuncType.QUERY, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='ids', type='GLuint*', wrap_params='n, ids'),
    ],
)

function(name='glGenPathsNV', enabled=True, function_type=FuncType.GEN,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='range', type='GLsizei'),
    ],
)

function(name='glGenPerfMonitorsAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='monitors', type='GLuint*'),
    ],
)

function(name='glGenProgramPipelines', enabled=True, function_type=FuncType.GEN, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='pipelines', type='GLuint*', wrap_type='CGLPipeline::CSMapArray', wrap_params='n, pipelines'),
    ],
)

function(name='glGenProgramPipelinesEXT', enabled=True, function_type=FuncType.GEN, inherit_from='glGenProgramPipelines')

function(name='glGenProgramsARB', enabled=True, function_type=FuncType.GEN,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='programs', type='GLuint*', wrap_type='CGLProgram::CSMapArray', wrap_params='n, programs'),
    ],
)

function(name='glGenProgramsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='programs', type='GLuint*'),
    ],
)

function(name='glGenQueries', enabled=True, function_type=FuncType.GEN|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='ids', type='GLuint*', wrap_type='CGLQuery::CSMapArray', wrap_params='n, ids'),
    ],
)

function(name='glGenQueriesARB', enabled=True, function_type=FuncType.GEN|FuncType.QUERY, inherit_from='glGenQueries')

function(name='glGenQueriesEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGenQueries')

function(name='glGenQueryResourceTagNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='tagIds', type='GLint*'),
    ],
)

function(name='glGenRenderbuffers', enabled=True, function_type=FuncType.GEN, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='renderbuffers', type='GLuint*', wrap_type='CGLRenderbuffer::CSMapArray', wrap_params='n, renderbuffers'),
    ],
)

function(name='glGenRenderbuffersEXT', enabled=True, function_type=FuncType.GEN, state_track=True, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='renderbuffers', type='GLuint*', wrap_type='CGLRenderbufferEXT::CSMapArray', wrap_params='n, renderbuffers'),
    ],
)

function(name='glGenRenderbuffersOES', enabled=True, function_type=FuncType.GEN, recorder_wrap=False, inherit_from='glGenRenderbuffersEXT')

function(name='glGenSamplers', enabled=True, function_type=FuncType.GEN, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='count', type='GLsizei'),
        Argument(name='samplers', type='GLuint*', wrap_type='CGLSampler::CSMapArray', wrap_params='count, samplers'),
    ],
)

function(name='glGenSemaphoresEXT', enabled=False, function_type=FuncType.GEN,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='semaphores', type='GLuint*'),
    ],
)

function(name='glGenSymbolsEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='datatype', type='GLenum'),
        Argument(name='storagetype', type='GLenum'),
        Argument(name='range', type='GLenum'),
        Argument(name='components', type='GLuint'),
    ],
)

function(name='glGenTextures', enabled=True, function_type=FuncType.GEN, state_track=True, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='textures', type='GLuint*', wrap_type='CGLTexture::CSMapArray', wrap_params='n, textures'),
    ],
)

function(name='glGenTexturesEXT', enabled=True, function_type=FuncType.GEN, inherit_from='glGenTextures')

function(name='glGenTransformFeedbacks', enabled=True, function_type=FuncType.GEN,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='ids', type='GLuint*', wrap_type='CGLTransformFeedback::CSMapArray', wrap_params='n, ids'),
    ],
)

function(name='glGenTransformFeedbacksNV', enabled=True, function_type=FuncType.GEN, inherit_from='glGenTransformFeedbacks')

function(name='glGenVertexArrays', enabled=True, function_type=FuncType.GEN, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='arrays', type='GLuint*', wrap_type='CGLVertexArray::CSMapArray', wrap_params='n, arrays'),
    ],
)

function(name='glGenVertexArraysAPPLE', enabled=False, function_type=FuncType.GEN,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='arrays', type='GLuint*'),
    ],
)

function(name='glGenVertexArraysOES', enabled=True, function_type=FuncType.GEN, inherit_from='glGenVertexArrays')

function(name='glGenVertexShadersEXT', enabled=True, function_type=FuncType.GEN,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='range', type='GLuint'),
    ],
)

function(name='glGenerateMipmap', enabled=True, function_type=FuncType.PARAM, state_track=True, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

function(name='glGenerateMipmapEXT', enabled=True, function_type=FuncType.PARAM, recorder_wrap=True, inherit_from='glGenerateMipmap')

function(name='glGenerateMipmapOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glGenerateMipmap')

function(name='glGenerateMultiTexMipmapEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
    ],
)

function(name='glGenerateTextureMipmap', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint'),
    ],
)

function(name='glGenerateTextureMipmapEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint'),
        Argument(name='target', type='GLenum'),
    ],
)

function(name='glGetActiveAtomicCounterBufferiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='bufferIndex', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetActiveAttrib', enabled=True, function_type=FuncType.GET,
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

function(name='glGetActiveAttribARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetActiveAttrib',
    args=[
        Argument(name='programObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='index', type='GLuint'),
        Argument(name='maxLength', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='size', type='GLint*', wrap_type='COutArgument'),
        Argument(name='type', type='GLenum*', wrap_type='COutArgument'),
        Argument(name='name', type='GLcharARB*', wrap_type='COutArgument'),
    ],
)

function(name='glGetActiveSubroutineName', enabled=True, function_type=FuncType.GET,
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

function(name='glGetActiveSubroutineUniformName', enabled=True, function_type=FuncType.GET,
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

function(name='glGetActiveSubroutineUniformiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='shadertype', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='values', type='GLint*'),
    ],
)

function(name='glGetActiveUniform', enabled=True, function_type=FuncType.GET,
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

function(name='glGetActiveUniformARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetActiveUniform',
    args=[
        Argument(name='programObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='index', type='GLuint'),
        Argument(name='maxLength', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='size', type='GLint*', wrap_type='COutArgument'),
        Argument(name='type', type='GLenum*', wrap_type='COutArgument'),
        Argument(name='name', type='GLcharARB*', wrap_type='COutArgument'),
    ],
)

function(name='glGetActiveUniformBlockName', enabled=True, function_type=FuncType.GET, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='uniformBlockIndex', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_params='1, length'),
        Argument(name='uniformBlockName', type='GLchar*', wrap_type='COutArgument'),
    ],
)

function(name='glGetActiveUniformBlockiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='uniformBlockIndex', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetActiveUniformName', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='uniformIndex', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='uniformName', type='GLchar*', wrap_type='COutArgument'),
    ],
)

function(name='glGetActiveUniformsiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='uniformCount', type='GLsizei'),
        Argument(name='uniformIndices', type='const GLuint*', wrap_params='uniformCount, uniformIndices'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetActiveVaryingNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetArrayObjectfvATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetArrayObjectivATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='array', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetAttachedObjectsARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='containerObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='maxCount', type='GLsizei'),
        Argument(name='count', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='obj', type='GLhandleARB*', wrap_type='COutArgument'),
    ],
)

function(name='glGetAttachedShaders', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='maxCount', type='GLsizei'),
        Argument(name='count', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='shaders', type='GLuint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetAttribLocation', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

function(name='glGetAttribLocationARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetAttribLocation',
    args=[
        Argument(name='programObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='name', type='const GLcharARB*', wrap_params='name, \'\\0\', 1'),
    ],
)

function(name='glGetBooleanIndexedvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLboolean*'),
    ],
)

function(name='glGetBooleani_v', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLboolean*', wrap_type='COutArgument'),
    ],
)

function(name='glGetBooleanv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='data', type='GLboolean*', wrap_type='COutArgument'),
    ],
)

function(name='glGetBufferParameteri64v', enabled=True, function_type=FuncType.GET, inherit_from='glGetBufferParameteriv',
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint64*', wrap_type='COutArgument'),
    ],
)

function(name='glGetBufferParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetBufferParameterivARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetBufferParameteriv')

function(name='glGetBufferParameterui64vNV', enabled=True, function_type=FuncType.GET, inherit_from='glGetBufferParameteriv',
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint64EXT*', wrap_type='COutArgument'),
    ],
)

function(name='glGetBufferPointerv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='void**', wrap_type='COutArgument'),
    ],
)

function(name='glGetBufferPointervARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetBufferPointerv')

function(name='glGetBufferPointervOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetBufferPointerv')

function(name='glGetBufferSubData', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='void*'),
    ],
)

function(name='glGetBufferSubDataARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetBufferSubData',
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='offset', type='GLintptrARB'),
        Argument(name='size', type='GLsizeiptrARB'),
        Argument(name='data', type='void*'),
    ],
)

function(name='glGetClipPlane', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='plane', type='GLenum'),
        Argument(name='equation', type='GLdouble*', wrap_type='COutArgument'),
    ],
)

function(name='glGetClipPlanef', enabled=True, function_type=FuncType.GET, inherit_from='glGetClipPlane',
    args=[
        Argument(name='plane', type='GLenum'),
        Argument(name='equation', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetClipPlanefOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetClipPlane',
    args=[
        Argument(name='plane', type='GLenum'),
        Argument(name='equation', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetClipPlanex', enabled=True, function_type=FuncType.GET, inherit_from='glGetClipPlane',
    args=[
        Argument(name='plane', type='GLenum'),
        Argument(name='equation', type='GLfixed*', wrap_type='COutArgument'),
    ],
)

function(name='glGetClipPlanexOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetClipPlane',
    args=[
        Argument(name='plane', type='GLenum'),
        Argument(name='equation', type='GLfixed*', wrap_type='COutArgument'),
    ],
)

function(name='glGetColorTable', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='table', type='void*'),
    ],
)

function(name='glGetColorTableEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetColorTable')

function(name='glGetColorTableParameterfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetColorTableParameterfvEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetColorTableParameterfv')

function(name='glGetColorTableParameterfvSGI', enabled=True, function_type=FuncType.GET, inherit_from='glGetColorTableParameterfv')

function(name='glGetColorTableParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetColorTableParameterivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetColorTableParameteriv')

function(name='glGetColorTableParameterivSGI', enabled=True, function_type=FuncType.GET, inherit_from='glGetColorTableParameteriv')

function(name='glGetColorTableSGI', enabled=False, function_type=FuncType.NONE, inherit_from='glGetColorTable')

function(name='glGetCombinerInputParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stage', type='GLenum'),
        Argument(name='portion', type='GLenum'),
        Argument(name='variable', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetCombinerInputParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stage', type='GLenum'),
        Argument(name='portion', type='GLenum'),
        Argument(name='variable', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetCombinerOutputParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stage', type='GLenum'),
        Argument(name='portion', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetCombinerOutputParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stage', type='GLenum'),
        Argument(name='portion', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetCombinerStageParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stage', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetCommandHeaderNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='tokenID', type='GLenum'),
        Argument(name='size', type='GLuint'),
    ],
)

function(name='glGetCompressedMultiTexImageEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='lod', type='GLint'),
        Argument(name='img', type='void*'),
    ],
)

function(name='glGetCompressedTexImage', enabled=True, function_type=FuncType.GET, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='img', type='void*', wrap_type='CGLvoid_ptr'),
    ],
)

function(name='glGetCompressedTexImageARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetCompressedTexImage')

function(name='glGetCompressedTextureImage', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='pixels', type='void*'),
    ],
)

function(name='glGetCompressedTextureImageEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='lod', type='GLint'),
        Argument(name='img', type='GLvoid*'),
    ],
)

function(name='glGetCompressedTextureSubImage', enabled=True, function_type=FuncType.PARAM,
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

function(name='glGetConvolutionFilter', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='image', type='void*'),
    ],
)

function(name='glGetConvolutionFilterEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetConvolutionFilter')

function(name='glGetConvolutionParameterfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetConvolutionParameterfvEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetConvolutionParameterfv')

function(name='glGetConvolutionParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetConvolutionParameterivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetConvolutionParameteriv')

function(name='glGetConvolutionParameterxvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*'),
    ],
)

function(name='glGetCoverageModulationTableNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='bufsize', type='GLsizei'),
        Argument(name='v', type='GLfloat*'),
    ],
)

function(name='glGetDebugMessageLog', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetDebugMessageLogAMD', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetDebugMessageLogARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetDebugMessageLog')

function(name='glGetDebugMessageLogKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glGetDebugMessageLog')

function(name='glGetDetailTexFuncSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='points', type='GLfloat*'),
    ],
)

function(name='glGetDoubleIndexedvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLdouble*'),
    ],
)

function(name='glGetDoublei_v', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLdouble*'),
    ],
)

function(name='glGetDoublei_vEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetDoublei_v')

function(name='glGetDoublev', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='data', type='GLdouble*', wrap_type='COutArgument'),
    ],
)

function(name='glGetDriverControlStringQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='driverControl', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='driverControlString', type='GLchar*'),
    ],
)

function(name='glGetDriverControlsQCOM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='num', type='GLint*'),
        Argument(name='size', type='GLsizei'),
        Argument(name='driverControls', type='GLuint*'),
    ],
)

function(name='glGetError', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLenum'),
    args=[],
)

function(name='glGetFenceivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='fence', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetFinalCombinerInputParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='variable', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetFinalCombinerInputParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='variable', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetFirstPerfQueryIdINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='queryId', type='GLuint*'),
    ],
)

function(name='glGetFixedv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*'),
    ],
)

function(name='glGetFixedvOES', enabled=False, function_type=FuncType.NONE, inherit_from='glGetFixedv')

function(name='glGetFloatIndexedvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLfloat*'),
    ],
)

function(name='glGetFloati_v', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLfloat*'),
    ],
)

function(name='glGetFloati_vEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetFloati_v')

function(name='glGetFloati_vNV', enabled=False, function_type=FuncType.NONE, inherit_from='glGetFloati_v')

function(name='glGetFloati_vOES', enabled=False, function_type=FuncType.NONE, inherit_from='glGetFloati_v')

function(name='glGetFloatv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='data', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetFogFuncSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='points', type='GLfloat*'),
    ],
)

function(name='glGetFragDataIndex', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

function(name='glGetFragDataIndexEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetFragDataIndex')

function(name='glGetFragDataLocation', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

function(name='glGetFragDataLocationEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetFragDataLocation')

function(name='glGetFragmentLightfvSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetFragmentLightivSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetFragmentMaterialfvSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetFragmentMaterialivSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetFramebufferAttachmentParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetFramebufferAttachmentParameterivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetFramebufferAttachmentParameteriv')

function(name='glGetFramebufferAttachmentParameterivOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetFramebufferAttachmentParameteriv')

function(name='glGetFramebufferParameterfvAMD', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetFramebufferParameteriv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetFramebufferParameterivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetFramebufferPixelLocalStorageSizeEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLsizei'),
    args=[
        Argument(name='target', type='GLuint'),
    ],
)

function(name='glGetGraphicsResetStatus', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLenum'),
    args=[],
)

function(name='glGetGraphicsResetStatusARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetGraphicsResetStatus')

function(name='glGetGraphicsResetStatusEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetGraphicsResetStatus')

function(name='glGetGraphicsResetStatusKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glGetGraphicsResetStatus')

function(name='glGetHandleARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLhandleARB'),
    args=[
        Argument(name='pname', type='GLenum'),
    ],
)

function(name='glGetHistogram', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='reset', type='GLboolean'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='values', type='void*'),
    ],
)

function(name='glGetHistogramEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetHistogram')

function(name='glGetHistogramParameterfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetHistogramParameterfvEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetHistogramParameterfv')

function(name='glGetHistogramParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetHistogramParameterivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetHistogramParameteriv')

function(name='glGetHistogramParameterxvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*'),
    ],
)

function(name='glGetImageHandleARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLuint64', wrap_type='CGLTextureHandle'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='layered', type='GLboolean'),
        Argument(name='layer', type='GLint'),
        Argument(name='format', type='GLenum'),
    ],
)

function(name='glGetImageHandleNV', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLuint64'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='layered', type='GLboolean'),
        Argument(name='layer', type='GLint'),
        Argument(name='format', type='GLenum'),
    ],
)

function(name='glGetImageTransformParameterfvHP', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetImageTransformParameterivHP', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetInfoLogARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='obj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='maxLength', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='infoLog', type='GLcharARB*', wrap_type='COutArgument'),
    ],
)

function(name='glGetInstrumentsSGIX', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLint'),
    args=[],
)

function(name='glGetInteger64i_v', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLint64*', wrap_type='COutArgument'),
    ],
)

function(name='glGetInteger64v', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='data', type='GLint64*', wrap_type='COutArgument'),
    ],
)

function(name='glGetInteger64vAPPLE', enabled=False, function_type=FuncType.GET, inherit_from='glGetInteger64v')

function(name='glGetIntegerIndexedvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLint*'),
    ],
)

function(name='glGetIntegeri_v', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetIntegeri_vEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetIntegeri_v')

function(name='glGetIntegerui64i_vNV', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='value', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='result', type='GLuint64EXT*', wrap_type='COutArgument'),
    ],
)

function(name='glGetIntegerui64vNV', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='value', type='GLenum'),
        Argument(name='result', type='GLuint64EXT*', wrap_type='COutArgument'),
    ],
)

function(name='glGetIntegerv', enabled=True, function_type=FuncType.GET, interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='data', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetInternalformatSampleivNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetInternalformati64v', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='params', type='GLint64*', wrap_type='COutArgument'),
    ],
)

function(name='glGetInternalformativ', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetInvariantBooleanvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='GLboolean*'),
    ],
)

function(name='glGetInvariantFloatvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='GLfloat*'),
    ],
)

function(name='glGetInvariantIntegervEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='GLint*'),
    ],
)

function(name='glGetLightfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetLightiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetLightxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*'),
    ],
)

function(name='glGetLightxv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*', wrap_type='COutArgument'),
    ],
)

function(name='glGetLightxvOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetLightxv')

function(name='glGetListParameterfvSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetListParameterivSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetLocalConstantBooleanvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='GLboolean*'),
    ],
)

function(name='glGetLocalConstantFloatvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='GLfloat*'),
    ],
)

function(name='glGetLocalConstantIntegervEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='GLint*'),
    ],
)

function(name='glGetMapAttribParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetMapAttribParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetMapControlPointsNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetMapParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetMapParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetMapdv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='query', type='GLenum'),
        Argument(name='v', type='GLdouble*', wrap_type='COutArgument'),
    ],
)

function(name='glGetMapfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='query', type='GLenum'),
        Argument(name='v', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetMapiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='query', type='GLenum'),
        Argument(name='v', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetMapxvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='query', type='GLenum'),
        Argument(name='v', type='GLfixed*'),
    ],
)

function(name='glGetMaterialfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetMaterialiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetMaterialxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

function(name='glGetMaterialxv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*', wrap_type='COutArgument'),
    ],
)

function(name='glGetMaterialxvOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetMaterialxv')

function(name='glGetMemoryObjectParameterivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='memoryObject', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetMinmax', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='reset', type='GLboolean'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='values', type='void*'),
    ],
)

function(name='glGetMinmaxEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetMinmax')

function(name='glGetMinmaxParameterfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetMinmaxParameterfvEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetMinmaxParameterfv')

function(name='glGetMinmaxParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetMinmaxParameterivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetMinmaxParameteriv')

function(name='glGetMultiTexEnvfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetMultiTexEnvivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetMultiTexGendvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLdouble*'),
    ],
)

function(name='glGetMultiTexGenfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetMultiTexGenivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetMultiTexImageEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetMultiTexLevelParameterfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetMultiTexLevelParameterivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetMultiTexParameterIivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetMultiTexParameterIuivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*'),
    ],
)

function(name='glGetMultiTexParameterfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetMultiTexParameterivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetMultisamplefv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='val', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetMultisamplefvNV', enabled=True, function_type=FuncType.GET, inherit_from='glGetMultisamplefv')

function(name='glGetNamedBufferParameteri64v', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint64*'),
    ],
)

function(name='glGetNamedBufferParameteriv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetNamedBufferParameterivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetNamedBufferParameteriv')

function(name='glGetNamedBufferParameterui64vNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint64EXT*'),
    ],
)

function(name='glGetNamedBufferPointerv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='void**', wrap_type='COutArgument'),
    ],
)

function(name='glGetNamedBufferPointervEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetNamedBufferPointerv')

function(name='glGetNamedBufferSubData', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='void*'),
    ],
)

function(name='glGetNamedBufferSubDataEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetNamedBufferSubData')

function(name='glGetNamedFramebufferAttachmentParameteriv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetNamedFramebufferAttachmentParameterivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetNamedFramebufferAttachmentParameteriv')

function(name='glGetNamedFramebufferParameterfvAMD', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetNamedFramebufferParameteriv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint*'),
    ],
)

function(name='glGetNamedFramebufferParameterivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetNamedFramebufferParameteriv')

function(name='glGetNamedProgramLocalParameterIivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetNamedProgramLocalParameterIuivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLuint*'),
    ],
)

function(name='glGetNamedProgramLocalParameterdvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLdouble*'),
    ],
)

function(name='glGetNamedProgramLocalParameterfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetNamedProgramStringEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='string', type='void*'),
    ],
)

function(name='glGetNamedProgramivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetNamedRenderbufferParameteriv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetNamedRenderbufferParameterivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetNamedRenderbufferParameteriv',
    args=[
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbufferEXT'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetNamedStringARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='namelen', type='GLint'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='stringlen', type='GLint*'),
        Argument(name='string', type='GLchar*'),
    ],
)

function(name='glGetNamedStringivARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='namelen', type='GLint'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetNextPerfQueryIdINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='queryId', type='GLuint'),
        Argument(name='nextQueryId', type='GLuint*'),
    ],
)

function(name='glGetObjectBufferfvATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetObjectBufferivATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetObjectLabel', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='identifier', type='GLenum'),
        Argument(name='name', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='label', type='GLchar*'),
    ],
)

function(name='glGetObjectLabelEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetObjectLabel')

function(name='glGetObjectLabelKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glGetObjectLabel')

function(name='glGetObjectParameterfvARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='obj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetObjectParameterivAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='objectType', type='GLenum'),
        Argument(name='name', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetObjectParameterivARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='obj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetObjectPtrLabel', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='ptr', type='const void*'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='label', type='GLchar*'),
    ],
)

function(name='glGetObjectPtrLabelKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glGetObjectPtrLabel')

function(name='glGetOcclusionQueryivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetOcclusionQueryuivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*'),
    ],
)

function(name='glGetPathColorGenfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='color', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLfloat*'),
    ],
)

function(name='glGetPathColorGenivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='color', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLint*'),
    ],
)

function(name='glGetPathCommandsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='commands', type='GLubyte*'),
    ],
)

function(name='glGetPathCoordsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='coords', type='GLfloat*'),
    ],
)

function(name='glGetPathDashArrayNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='dashArray', type='GLfloat*'),
    ],
)

function(name='glGetPathLengthNV', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLfloat'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='startSegment', type='GLsizei'),
        Argument(name='numSegments', type='GLsizei'),
    ],
)

function(name='glGetPathMetricRangeNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='metricQueryMask', type='GLbitfield'),
        Argument(name='firstPathName', type='GLuint'),
        Argument(name='numPaths', type='GLsizei'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='metrics', type='GLfloat*'),
    ],
)

function(name='glGetPathMetricsNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetPathParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLfloat*'),
    ],
)

function(name='glGetPathParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLint*'),
    ],
)

function(name='glGetPathSpacingNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetPathTexGenfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texCoordSet', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLfloat*'),
    ],
)

function(name='glGetPathTexGenivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texCoordSet', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLint*'),
    ],
)

function(name='glGetPerfCounterInfoINTEL', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetPerfMonitorCounterDataAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='monitor', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='dataSize', type='GLsizei'),
        Argument(name='data', type='GLuint*'),
        Argument(name='bytesWritten', type='GLint*'),
    ],
)

function(name='glGetPerfMonitorCounterInfoAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='group', type='GLuint'),
        Argument(name='counter', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='data', type='void*'),
    ],
)

function(name='glGetPerfMonitorCounterStringAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='group', type='GLuint'),
        Argument(name='counter', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='counterString', type='GLchar*'),
    ],
)

function(name='glGetPerfMonitorCountersAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='group', type='GLuint'),
        Argument(name='numCounters', type='GLint*'),
        Argument(name='maxActiveCounters', type='GLint*'),
        Argument(name='counterSize', type='GLsizei'),
        Argument(name='counters', type='GLuint*'),
    ],
)

function(name='glGetPerfMonitorGroupStringAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='group', type='GLuint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='groupString', type='GLchar*'),
    ],
)

function(name='glGetPerfMonitorGroupsAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='numGroups', type='GLint*'),
        Argument(name='groupsSize', type='GLsizei'),
        Argument(name='groups', type='GLuint*'),
    ],
)

function(name='glGetPerfQueryDataINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='queryHandle', type='GLuint'),
        Argument(name='flags', type='GLuint'),
        Argument(name='dataSize', type='GLsizei'),
        Argument(name='data', type='void*'),
        Argument(name='bytesWritten', type='GLuint*'),
    ],
)

function(name='glGetPerfQueryIdByNameINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='queryName', type='GLchar*'),
        Argument(name='queryId', type='GLuint*'),
    ],
)

function(name='glGetPerfQueryInfoINTEL', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetPixelMapfv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='values', type='GLfloat*'),
    ],
)

function(name='glGetPixelMapuiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='values', type='GLuint*'),
    ],
)

function(name='glGetPixelMapusv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='values', type='GLushort*'),
    ],
)

function(name='glGetPixelMapxv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='size', type='GLint'),
        Argument(name='values', type='GLfixed*'),
    ],
)

function(name='glGetPixelTexGenParameterfvSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetPixelTexGenParameterivSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetPixelTransformParameterfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetPixelTransformParameterivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetPointerIndexedvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='void**'),
    ],
)

function(name='glGetPointeri_vEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='void**'),
    ],
)

function(name='glGetPointerv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='void**', wrap_type='COutArgument'),
    ],
)

function(name='glGetPointervEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetPointerv')

function(name='glGetPointervKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glGetPointerv')

function(name='glGetPolygonStipple', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLubyte*', wrap_type='COutArgument'),
    ],
)

function(name='glGetProgramBinary', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='binaryFormat', type='GLenum*'),
        Argument(name='binary', type='void*'),
    ],
)

function(name='glGetProgramBinaryOES', enabled=False, function_type=FuncType.NONE, inherit_from='glGetProgramBinary')

function(name='glGetProgramEnvParameterIivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetProgramEnvParameterIuivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLuint*'),
    ],
)

function(name='glGetProgramEnvParameterdvARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLdouble*', wrap_type='COutArgument'),
    ],
)

function(name='glGetProgramEnvParameterfvARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetProgramInfoLog', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='infoLog', type='GLchar*', wrap_type='COutArgument'),
    ],
)

function(name='glGetProgramInterfaceiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='programInterface', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetProgramLocalParameterIivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetProgramLocalParameterIuivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLuint*'),
    ],
)

function(name='glGetProgramLocalParameterdvARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLdouble*', wrap_type='COutArgument'),
    ],
)

function(name='glGetProgramLocalParameterfvARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetProgramNamedParameterdvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='len', type='GLsizei'),
        Argument(name='name', type='const GLubyte*'),
        Argument(name='params', type='GLdouble*'),
    ],
)

function(name='glGetProgramNamedParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='len', type='GLsizei'),
        Argument(name='name', type='const GLubyte*'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetProgramParameterdvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLdouble*'),
    ],
)

function(name='glGetProgramParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetProgramPipelineInfoLog', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pipeline', type='GLuint', wrap_type='CGLPipeline'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='infoLog', type='GLchar*'),
    ],
)

function(name='glGetProgramPipelineInfoLogEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetProgramPipelineInfoLog')

function(name='glGetProgramPipelineiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pipeline', type='GLuint', wrap_type='CGLPipeline'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetProgramPipelineivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetProgramPipelineiv')

function(name='glGetProgramResourceIndex', enabled=True, function_type=FuncType.GET, run_wrap=True,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='programInterface', type='GLenum'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

function(name='glGetProgramResourceLocation', enabled=True, function_type=FuncType.GET_ESSENTIAL, run_wrap=True,
    return_value=ReturnValue(type='GLint', wrap_type='CRecUniformLocation', wrap_params='programInterface == GL_UNIFORM ? return_value : -1, program, name'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='programInterface', type='GLenum'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

function(name='glGetProgramResourceLocationIndex', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='programInterface', type='GLenum'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

function(name='glGetProgramResourceLocationIndexEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetProgramResourceLocationIndex')

function(name='glGetProgramResourceName', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetProgramResourcefvNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetProgramResourceiv', enabled=True, function_type=FuncType.GET_ESSENTIAL, run_wrap=True,
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

function(name='glGetProgramStageiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='shadertype', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='values', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetProgramStringARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='string', type='void*'),
    ],
)

function(name='glGetProgramStringNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='program', type='GLubyte*'),
    ],
)

function(name='glGetProgramSubroutineParameteruivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='param', type='GLuint*'),
    ],
)

function(name='glGetProgramiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetProgramivARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetProgramivNV', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetQueryBufferObjecti64v', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
    ],
)

function(name='glGetQueryBufferObjectiv', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
    ],
)

function(name='glGetQueryBufferObjectui64v', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
    ],
)

function(name='glGetQueryBufferObjectuiv', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
    ],
)

function(name='glGetQueryIndexediv', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetQueryObjecti64v', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint64*', wrap_type='COutArgument'),
    ],
)

function(name='glGetQueryObjecti64vEXT', enabled=True, function_type=FuncType.GET|FuncType.QUERY, inherit_from='glGetQueryObjecti64v',
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint64EXT*', wrap_type='COutArgument'),
    ],
)

function(name='glGetQueryObjecti64vINTEL', enabled=True, function_type=FuncType.GET|FuncType.QUERY, inherit_from='glGetQueryObjecti64v')

function(name='glGetQueryObjectiv', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetQueryObjectivARB', enabled=True, function_type=FuncType.GET|FuncType.QUERY, inherit_from='glGetQueryObjectiv')

function(name='glGetQueryObjectivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetQueryObjectiv')

function(name='glGetQueryObjectui64v', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint64*', wrap_type='COutArgument'),
    ],
)

function(name='glGetQueryObjectui64vEXT', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint64EXT*', wrap_type='COutArgument'),
    ],
)

function(name='glGetQueryObjectui64vINTEL', enabled=True, function_type=FuncType.GET|FuncType.QUERY, inherit_from='glGetQueryObjectui64v')

function(name='glGetQueryObjectuiv', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetQueryObjectuivARB', enabled=True, function_type=FuncType.GET|FuncType.QUERY, inherit_from='glGetQueryObjectuiv')

function(name='glGetQueryObjectuivEXT', enabled=True, function_type=FuncType.GET|FuncType.QUERY, inherit_from='glGetQueryObjectuiv')

function(name='glGetQueryiv', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetQueryivARB', enabled=True, function_type=FuncType.GET|FuncType.QUERY, inherit_from='glGetQueryiv')

function(name='glGetQueryivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetQueryiv')

function(name='glGetRenderbufferParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetRenderbufferParameterivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetRenderbufferParameteriv')

function(name='glGetRenderbufferParameterivOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetRenderbufferParameteriv')

function(name='glGetSamplerParameterIiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetSamplerParameterIivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetSamplerParameterIiv')

function(name='glGetSamplerParameterIivOES', enabled=False, function_type=FuncType.NONE, inherit_from='glGetSamplerParameterIiv')

function(name='glGetSamplerParameterIuiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetSamplerParameterIuivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetSamplerParameterIuiv')

function(name='glGetSamplerParameterIuivOES', enabled=False, function_type=FuncType.NONE, inherit_from='glGetSamplerParameterIuiv')

function(name='glGetSamplerParameterfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetSamplerParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetSemaphoreParameterivNV', enabled=False, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='semaphore', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetSemaphoreParameterui64vEXT', enabled=False, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='semaphore', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint64*'),
    ],
)

function(name='glGetSeparableFilter', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetSeparableFilterEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetSeparableFilter')

function(name='glGetShaderInfoLog', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='infoLog', type='GLchar*', wrap_type='COutArgument'),
    ],
)

function(name='glGetShaderPrecisionFormat', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shadertype', type='GLenum'),
        Argument(name='precisiontype', type='GLenum'),
        Argument(name='range', type='GLint*', wrap_type='COutArgument'),
        Argument(name='precision', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetShaderSource', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='source', type='GLchar*', wrap_type='COutArgument'),
    ],
)

function(name='glGetShaderSourceARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='obj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='maxLength', type='GLsizei'),
        Argument(name='length', type='GLsizei*', wrap_type='COutArgument'),
        Argument(name='source', type='GLcharARB*', wrap_type='COutArgument'),
    ],
)

function(name='glGetShaderiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shader', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetSharpenTexFuncSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='points', type='GLfloat*'),
    ],
)

function(name='glGetStageIndexNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLushort'),
    args=[
        Argument(name='shadertype', type='GLenum'),
    ],
)

function(name='glGetString', enabled=True, function_type=FuncType.GET, interceptor_exec_override=True,
    return_value=ReturnValue(type='const GLubyte*', wrap_type='CGLconstuchar_ptr'),
    args=[
        Argument(name='name', type='GLenum'),
    ],
)

function(name='glGetStringi', enabled=True, function_type=FuncType.GET, interceptor_exec_override=True,
    return_value=ReturnValue(type='const GLubyte*', wrap_type='CGLconstuchar_ptr'),
    args=[
        Argument(name='name', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

function(name='glGetSubroutineIndex', enabled=True, function_type=FuncType.GET, run_wrap=True,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='shadertype', type='GLenum'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

function(name='glGetSubroutineUniformLocation', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='shadertype', type='GLenum'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

function(name='glGetSynciv', enabled=True, function_type=FuncType.GET, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sync', type='GLsync'),
        Argument(name='pname', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='values', type='GLint*', wrap_params='bufSize/sizeof(GLint), values'),
    ],
)

function(name='glGetSyncivAPPLE', enabled=False, function_type=FuncType.NONE, inherit_from='glGetSynciv')

function(name='glGetTexBumpParameterfvATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat*'),
    ],
)

function(name='glGetTexBumpParameterivATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint*'),
    ],
)

function(name='glGetTexEnvfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetTexEnviv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetTexEnvxv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*', wrap_type='COutArgument'),
    ],
)

function(name='glGetTexEnvxvOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetTexEnvxv')

function(name='glGetTexFilterFuncSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='filter', type='GLenum'),
        Argument(name='weights', type='GLfloat*'),
    ],
)

function(name='glGetTexGendv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLdouble*', wrap_type='COutArgument'),
    ],
)

function(name='glGetTexGenfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetTexGenfvOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetTexGenfv')

function(name='glGetTexGeniv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetTexGenivOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetTexGeniv')

function(name='glGetTexGenxvOES', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*', wrap_type='COutArgument'),
    ],
)

function(name='glGetTexImage', enabled=True, function_type=FuncType.GET, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='pixels', type='void*', wrap_type='CGLvoid_ptr'),
    ],
)

function(name='glGetTexLevelParameterfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetTexLevelParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetTexLevelParameterxvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*'),
    ],
)

function(name='glGetTexParameterIiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetTexParameterIivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetTexParameterIiv')

function(name='glGetTexParameterIivOES', enabled=False, function_type=FuncType.NONE, inherit_from='glGetTexParameterIiv')

function(name='glGetTexParameterIuiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetTexParameterIuivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetTexParameterIuiv')

function(name='glGetTexParameterIuivOES', enabled=False, function_type=FuncType.NONE, inherit_from='glGetTexParameterIuiv')

function(name='glGetTexParameterPointervAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='void**'),
    ],
)

function(name='glGetTexParameterfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetTexParameteriv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetTexParameterxv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfixed*', wrap_type='COutArgument'),
    ],
)

function(name='glGetTexParameterxvOES', enabled=True, function_type=FuncType.GET, inherit_from='glGetTexParameterxv')

function(name='glGetTextureHandleARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLuint64', wrap_type='CGLTextureHandle'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
    ],
)

function(name='glGetTextureHandleIMG', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLuint64'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
    ],
)

function(name='glGetTextureHandleNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLuint64'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
    ],
)

function(name='glGetTextureImage', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetTextureImageEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetTextureLevelParameterfv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetTextureLevelParameterfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetTextureLevelParameteriv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetTextureLevelParameterivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='level', type='GLint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetTextureParameterIiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetTextureParameterIivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetTextureParameterIuiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*'),
    ],
)

function(name='glGetTextureParameterIuivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*'),
    ],
)

function(name='glGetTextureParameterfv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetTextureParameterfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetTextureParameteriv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetTextureParameterivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetTextureSamplerHandleARB', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLuint64', wrap_type='CGLTextureHandle'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
    ],
)

function(name='glGetTextureSamplerHandleIMG', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLuint64'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
    ],
)

function(name='glGetTextureSamplerHandleNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLuint64'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
    ],
)

function(name='glGetTextureSubImage', enabled=True, function_type=FuncType.PARAM, run_wrap=True,
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

function(name='glGetTrackMatrixivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='address', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetTransformFeedbackVarying', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetTransformFeedbackVaryingEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetTransformFeedbackVarying')

function(name='glGetTransformFeedbackVaryingNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='index', type='GLuint'),
        Argument(name='location', type='GLint*'),
    ],
)

function(name='glGetTransformFeedbacki64_v', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='xfb', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='param', type='GLint64*'),
    ],
)

function(name='glGetTransformFeedbacki_v', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='xfb', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='param', type='GLint*'),
    ],
)

function(name='glGetTransformFeedbackiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='xfb', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint*'),
    ],
)

function(name='glGetTranslatedShaderSourceANGLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='bufsize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='source', type='GLchar*'),
    ],
)

function(name='glGetUniformBlockIndex', enabled=True, function_type=FuncType.GET, run_wrap=True,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='uniformBlockName', type='const GLchar*', wrap_params='uniformBlockName, \'\\0\', 1'),
    ],
)

function(name='glGetUniformBufferSizeEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
    ],
)

function(name='glGetUniformIndices', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='uniformCount', type='GLsizei'),
        Argument(name='uniformNames', type='const GLchar*const*', wrap_type='CStringArray', wrap_params='uniformNames, uniformCount'),
        Argument(name='uniformIndices', type='GLuint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetUniformLocation', enabled=True, function_type=FuncType.GET_ESSENTIAL, run_wrap=True,
    return_value=ReturnValue(type='GLint', wrap_type='CRecUniformLocation', wrap_params='return_value, program, name'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

function(name='glGetUniformLocationARB', enabled=True, function_type=FuncType.GET_ESSENTIAL, run_wrap=True,
    return_value=ReturnValue(type='GLint', wrap_type='CRecUniformLocation', wrap_params='return_value, programObj, name'),
    args=[
        Argument(name='programObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='name', type='const GLcharARB*', wrap_type='CGLchar::CSArray', wrap_params='name, \'\\0\', 1'),
    ],
)

function(name='glGetUniformOffsetEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLintptr'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
    ],
)

function(name='glGetUniformSubroutineuiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shadertype', type='GLenum'),
        Argument(name='location', type='GLint'),
        Argument(name='params', type='GLuint*'),
    ],
)

function(name='glGetUniformdv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='params', type='GLdouble*', wrap_type='COutArgument'),
    ],
)

function(name='glGetUniformfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetUniformfvARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetUniformfv',
    args=[
        Argument(name='programObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='programObj, location'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetUniformi64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='params', type='GLint64*'),
    ],
)

function(name='glGetUniformi64vNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='params', type='GLint64EXT*'),
    ],
)

function(name='glGetUniformiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetUniformivARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetUniformiv',
    args=[
        Argument(name='programObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='programObj, location'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetUniformui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='params', type='GLuint64*'),
    ],
)

function(name='glGetUniformui64vNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='params', type='GLuint64EXT*'),
    ],
)

function(name='glGetUniformuiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='params', type='GLuint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetUniformuivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetUniformuiv')

function(name='glGetUnsignedBytei_vEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='data', type='GLubyte*'),
    ],
)

function(name='glGetUnsignedBytevEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='data', type='GLubyte*'),
    ],
)

function(name='glGetVariantArrayObjectfvATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetVariantArrayObjectivATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetVariantBooleanvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='GLboolean*'),
    ],
)

function(name='glGetVariantFloatvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='GLfloat*'),
    ],
)

function(name='glGetVariantIntegervEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='GLint*'),
    ],
)

function(name='glGetVariantPointervEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='value', type='GLenum'),
        Argument(name='data', type='void**'),
    ],
)

function(name='glGetVaryingLocationNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

function(name='glGetVertexArrayIndexed64iv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint64*', wrap_type='CGLint64_ptr'),
    ],
)

function(name='glGetVertexArrayIndexediv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetVertexArrayIntegeri_vEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint*'),
    ],
)

function(name='glGetVertexArrayIntegervEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint*'),
    ],
)

function(name='glGetVertexArrayPointeri_vEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='void**'),
    ],
)

function(name='glGetVertexArrayPointervEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='void**'),
    ],
)

function(name='glGetVertexArrayiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetVertexAttribArrayObjectfvATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetVertexAttribArrayObjectivATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetVertexAttribIiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetVertexAttribIivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribIiv')

function(name='glGetVertexAttribIuiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetVertexAttribIuivEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribIuiv')

function(name='glGetVertexAttribLdv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLdouble*', wrap_type='COutArgument'),
    ],
)

function(name='glGetVertexAttribLdvEXT', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribLdv')

function(name='glGetVertexAttribLi64vNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint64EXT*'),
    ],
)

function(name='glGetVertexAttribLui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint64EXT*'),
    ],
)

function(name='glGetVertexAttribLui64vNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint64EXT*'),
    ],
)

function(name='glGetVertexAttribPointerv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='pointer', type='void**', wrap_type='COutArgument'),
    ],
)

function(name='glGetVertexAttribPointervARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribPointerv')

function(name='glGetVertexAttribPointervNV', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribPointerv')

function(name='glGetVertexAttribdv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLdouble*', wrap_type='COutArgument'),
    ],
)

function(name='glGetVertexAttribdvARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribdv')

function(name='glGetVertexAttribdvNV', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribdv')

function(name='glGetVertexAttribfv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*', wrap_type='COutArgument'),
    ],
)

function(name='glGetVertexAttribfvARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribfv')

function(name='glGetVertexAttribfvNV', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribfv')

function(name='glGetVertexAttribiv', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*', wrap_type='COutArgument'),
    ],
)

function(name='glGetVertexAttribivARB', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribiv')

function(name='glGetVertexAttribivNV', enabled=True, function_type=FuncType.GET, inherit_from='glGetVertexAttribiv')

function(name='glGetVideoCaptureStreamdvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='stream', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLdouble*'),
    ],
)

function(name='glGetVideoCaptureStreamfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='stream', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetVideoCaptureStreamivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='stream', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetVideoCaptureivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetVideoi64vNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_slot', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint64EXT*'),
    ],
)

function(name='glGetVideoivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_slot', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetVideoui64vNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_slot', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint64EXT*'),
    ],
)

function(name='glGetVideouivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_slot', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*'),
    ],
)

function(name='glGetVkProcAddrNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLVULKANPROCNV'),
    args=[
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

function(name='glGetnColorTable', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='table', type='void*'),
    ],
)

function(name='glGetnColorTableARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnColorTable')

function(name='glGetnCompressedTexImage', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='lod', type='GLint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='pixels', type='void*', wrap_type='CGLvoid_ptr'),
    ],
)

function(name='glGetnCompressedTexImageARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnCompressedTexImage')

function(name='glGetnConvolutionFilter', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='image', type='void*'),
    ],
)

function(name='glGetnConvolutionFilterARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnConvolutionFilter')

function(name='glGetnHistogram', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetnHistogramARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnHistogram')

function(name='glGetnMapdv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='query', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='v', type='GLdouble*'),
    ],
)

function(name='glGetnMapdvARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnMapdv')

function(name='glGetnMapfv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='query', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='v', type='GLfloat*'),
    ],
)

function(name='glGetnMapfvARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnMapfv')

function(name='glGetnMapiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='query', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='v', type='GLint*'),
    ],
)

function(name='glGetnMapivARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnMapiv')

function(name='glGetnMinmax', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetnMinmaxARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnMinmax')

function(name='glGetnPixelMapfv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='values', type='GLfloat*'),
    ],
)

function(name='glGetnPixelMapfvARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnPixelMapfv')

function(name='glGetnPixelMapuiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='values', type='GLuint*'),
    ],
)

function(name='glGetnPixelMapuivARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnPixelMapuiv')

function(name='glGetnPixelMapusv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='values', type='GLushort*'),
    ],
)

function(name='glGetnPixelMapusvARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnPixelMapusv')

function(name='glGetnPolygonStipple', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='pattern', type='GLubyte*'),
    ],
)

function(name='glGetnPolygonStippleARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnPolygonStipple')

function(name='glGetnSeparableFilter', enabled=False, function_type=FuncType.NONE,
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

function(name='glGetnSeparableFilterARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnSeparableFilter')

function(name='glGetnTexImage', enabled=True, function_type=FuncType.PARAM,
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

function(name='glGetnTexImageARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnTexImage')

function(name='glGetnUniformdv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='params', type='GLdouble*'),
    ],
)

function(name='glGetnUniformdvARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnUniformdv')

function(name='glGetnUniformfv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='params', type='GLfloat*'),
    ],
)

function(name='glGetnUniformfvARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnUniformfv')

function(name='glGetnUniformfvEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnUniformfv')

function(name='glGetnUniformfvKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnUniformfv')

function(name='glGetnUniformi64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='params', type='GLint64*'),
    ],
)

function(name='glGetnUniformiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glGetnUniformivARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnUniformiv')

function(name='glGetnUniformivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnUniformiv')

function(name='glGetnUniformivKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnUniformiv')

function(name='glGetnUniformui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='params', type='GLuint64*'),
    ],
)

function(name='glGetnUniformuiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='params', type='GLuint*'),
    ],
)

function(name='glGetnUniformuivARB', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnUniformuiv')

function(name='glGetnUniformuivKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glGetnUniformuiv')

function(name='glGlobalAlphaFactorbSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLbyte'),
    ],
)

function(name='glGlobalAlphaFactordSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLdouble'),
    ],
)

function(name='glGlobalAlphaFactorfSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLfloat'),
    ],
)

function(name='glGlobalAlphaFactoriSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLint'),
    ],
)

function(name='glGlobalAlphaFactorsSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLshort'),
    ],
)

function(name='glGlobalAlphaFactorubSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLubyte'),
    ],
)

function(name='glGlobalAlphaFactoruiSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLuint'),
    ],
)

function(name='glGlobalAlphaFactorusSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLushort'),
    ],
)

function(name='glHint', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glHintPGI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='mode', type='GLint'),
    ],
)

function(name='glHistogram', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='sink', type='GLboolean'),
    ],
)

function(name='glHistogramEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glHistogram')

function(name='glIglooInterfaceSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const void*'),
    ],
)

function(name='glImageTransformParameterfHP', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glImageTransformParameterfvHP', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

function(name='glImageTransformParameteriHP', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glImageTransformParameterivHP', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glImportMemoryFdEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='memory', type='GLuint'),
        Argument(name='size', type='GLuint64'),
        Argument(name='handleType', type='GLenum'),
        Argument(name='fd', type='GLint'),
    ],
)

function(name='glImportMemoryWin32HandleEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='memory', type='GLuint'),
        Argument(name='size', type='GLuint64'),
        Argument(name='handleType', type='GLenum'),
        Argument(name='handle', type='void*'),
    ],
)

function(name='glImportMemoryWin32NameEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='memory', type='GLuint'),
        Argument(name='size', type='GLuint64'),
        Argument(name='handleType', type='GLenum'),
        Argument(name='name', type='const void*'),
    ],
)

function(name='glImportSemaphoreFdEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='semaphore', type='GLuint'),
        Argument(name='handleType', type='GLenum'),
        Argument(name='fd', type='GLint'),
    ],
)

function(name='glImportSemaphoreWin32HandleEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='semaphore', type='GLuint'),
        Argument(name='handleType', type='GLenum'),
        Argument(name='handle', type='void*'),
    ],
)

function(name='glImportSemaphoreWin32NameEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='semaphore', type='GLuint'),
        Argument(name='handleType', type='GLenum'),
        Argument(name='name', type='const void*'),
    ],
)

function(name='glImportSyncEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLsync'),
    args=[
        Argument(name='external_sync_type', type='GLenum'),
        Argument(name='external_sync', type='GLintptr'),
        Argument(name='flags', type='GLbitfield'),
    ],
)

function(name='glIndexFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
    ],
)

function(name='glIndexFuncEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='func', type='GLenum'),
        Argument(name='ref', type='GLclampf'),
    ],
)

function(name='glIndexMask', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLuint'),
    ],
)

function(name='glIndexMaterialEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glIndexPointer', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glIndexPointerEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='count', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glIndexPointerListIBM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLint'),
        Argument(name='pointer', type='const void**'),
        Argument(name='ptrstride', type='GLint'),
    ],
)

function(name='glIndexd', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='GLdouble'),
    ],
)

function(name='glIndexdv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='const GLdouble*'),
    ],
)

function(name='glIndexf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='GLfloat'),
    ],
)

function(name='glIndexfv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='const GLfloat*'),
    ],
)

function(name='glIndexi', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='GLint'),
    ],
)

function(name='glIndexiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='const GLint*'),
    ],
)

function(name='glIndexs', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='GLshort'),
    ],
)

function(name='glIndexsv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='const GLshort*'),
    ],
)

function(name='glIndexub', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='GLubyte'),
    ],
)

function(name='glIndexubv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='c', type='const GLubyte*'),
    ],
)

function(name='glIndexxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='component', type='GLfixed'),
    ],
)

function(name='glIndexxvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='component', type='const GLfixed*'),
    ],
)

function(name='glInitNames', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glInsertComponentEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='res', type='GLuint'),
        Argument(name='src', type='GLuint'),
        Argument(name='num', type='GLuint'),
    ],
)

function(name='glInsertEventMarkerEXT', enabled=False, function_type=FuncType.NONE, interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='length', type='GLsizei'),
        Argument(name='marker', type='const GLchar*'),
    ],
)

function(name='glInstrumentsBufferSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLsizei'),
        Argument(name='buffer', type='GLint*'),
    ],
)

function(name='glInterleavedArrays', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='format', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

function(name='glInterpolatePathsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='resultPath', type='GLuint'),
        Argument(name='pathA', type='GLuint'),
        Argument(name='pathB', type='GLuint'),
        Argument(name='weight', type='GLfloat'),
    ],
)

function(name='glInvalidateBufferData', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

function(name='glInvalidateBufferSubData', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='length', type='GLsizeiptr'),
    ],
)

function(name='glInvalidateFramebuffer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='numAttachments', type='GLsizei'),
        Argument(name='attachments', type='const GLenum*', wrap_params='numAttachments, attachments'),
    ],
)

function(name='glInvalidateNamedFramebufferData', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='numAttachments', type='GLsizei'),
        Argument(name='attachments', type='const GLenum*', wrap_params='numAttachments, attachments'),
    ],
)

function(name='glInvalidateNamedFramebufferSubData', enabled=False, function_type=FuncType.NONE,
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

function(name='glInvalidateSubFramebuffer', enabled=False, function_type=FuncType.NONE,
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

function(name='glInvalidateTexImage', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
    ],
)

function(name='glInvalidateTexSubImage', enabled=False, function_type=FuncType.NONE,
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

function(name='glIsAsyncMarkerSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='marker', type='GLuint'),
    ],
)

function(name='glIsBuffer', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

function(name='glIsBufferARB', enabled=True, function_type=FuncType.GET, inherit_from='glIsBuffer')

function(name='glIsBufferResidentNV', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

function(name='glIsCommandListNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='list', type='GLuint'),
    ],
)

function(name='glIsEnabled', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='cap', type='GLenum'),
    ],
)

function(name='glIsEnabledIndexedEXT', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

function(name='glIsEnabledi', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
    ],
)

function(name='glIsEnablediEXT', enabled=True, function_type=FuncType.GET, inherit_from='glIsEnabledi')

function(name='glIsEnablediNV', enabled=False, function_type=FuncType.NONE, inherit_from='glIsEnabledi')

function(name='glIsEnablediOES', enabled=False, function_type=FuncType.NONE, inherit_from='glIsEnabledi')

function(name='glIsFenceAPPLE', enabled=False, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='fence', type='GLuint'),
    ],
)

# FIXME: Argument `fence` had a `wrapParam='CGLfence'`, which was a typo of wrapType.
# FIXME: Should we add that wrap_type or not?
# Adding `wrapParam='CGLfence'` causes CglIsFenceNV in glFunctions.h to contain CGLfence instead of CGLuint.
function(name='glIsFenceNV', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='fence', type='GLuint'),
    ],
)


function(name='glIsFramebuffer', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
    ],
)

function(name='glIsFramebufferEXT', enabled=True, function_type=FuncType.GET, recorder_wrap=True,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebufferEXT'),
    ],
)

function(name='glIsFramebufferOES', enabled=True, function_type=FuncType.GET, inherit_from='glIsFramebuffer')

function(name='glIsImageHandleResidentARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='handle', type='GLuint64'),
    ],
)

function(name='glIsImageHandleResidentNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='handle', type='GLuint64'),
    ],
)

function(name='glIsList', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='list', type='GLuint'),
    ],
)

function(name='glIsMemoryObjectEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='memoryObject', type='GLuint'),
    ],
)

function(name='glIsNameAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='identifier', type='GLenum'),
        Argument(name='name', type='GLuint'),
    ],
)

function(name='glIsNamedBufferResidentNV', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

function(name='glIsNamedStringARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='namelen', type='GLint'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
    ],
)

function(name='glIsObjectBufferATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

function(name='glIsOcclusionQueryNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='id', type='GLuint'),
    ],
)

function(name='glIsPathNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='path', type='GLuint'),
    ],
)

function(name='glIsPointInFillPathNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='mask', type='GLuint'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
    ],
)

function(name='glIsPointInStrokePathNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
    ],
)

function(name='glIsProgram', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

function(name='glIsProgramARB', enabled=True, function_type=FuncType.GET, inherit_from='glIsProgram')

function(name='glIsProgramNV', enabled=True, function_type=FuncType.GET, inherit_from='glIsProgram')

function(name='glIsProgramPipeline', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='pipeline', type='GLuint', wrap_type='CGLPipeline'),
    ],
)

function(name='glIsProgramPipelineEXT', enabled=True, function_type=FuncType.GET, inherit_from='glIsProgramPipeline')

function(name='glIsQuery', enabled=True, function_type=FuncType.GET|FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
    ],
)

function(name='glIsQueryARB', enabled=True, function_type=FuncType.GET, inherit_from='glIsQuery')

function(name='glIsQueryEXT', enabled=True, function_type=FuncType.GET, inherit_from='glIsQuery')

function(name='glIsRenderbuffer', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
    ],
)

function(name='glIsRenderbufferEXT', enabled=True, function_type=FuncType.GET, recorder_wrap=True, inherit_from='glIsRenderbuffer',
    args=[
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbufferEXT'),
    ],
)

function(name='glIsRenderbufferOES', enabled=True, function_type=FuncType.GET, inherit_from='glIsRenderbuffer')

function(name='glIsSampler', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
    ],
)

function(name='glIsSemaphoreEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='semaphore', type='GLuint'),
    ],
)

function(name='glIsShader', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
    ],
)

function(name='glIsStateNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='state', type='GLuint'),
    ],
)

function(name='glIsSync', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='sync', type='GLsync', wrap_type='CGLsync'),
    ],
)

function(name='glIsSyncAPPLE', enabled=False, function_type=FuncType.GET, inherit_from='glIsSync')

function(name='glIsTexture', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
    ],
)

function(name='glIsTextureEXT', enabled=True, function_type=FuncType.GET, inherit_from='glIsTexture', run_wrap=False, recorder_wrap=False)

function(name='glIsTextureHandleResidentARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='handle', type='GLuint64'),
    ],
)

function(name='glIsTextureHandleResidentNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='handle', type='GLuint64'),
    ],
)

function(name='glIsTransformFeedback', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLTransformFeedback'),
    ],
)

function(name='glIsTransformFeedbackNV', enabled=True, function_type=FuncType.GET, inherit_from='glIsTransformFeedback')

function(name='glIsVariantEnabledEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='cap', type='GLenum'),
    ],
)

function(name='glIsVertexArray', enabled=True, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='array', type='GLuint', wrap_type='CGLVertexArray'),
    ],
)

function(name='glIsVertexArrayAPPLE', enabled=False, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='array', type='GLuint'),
    ],
)

function(name='glIsVertexArrayOES', enabled=True, function_type=FuncType.GET, inherit_from='glIsVertexArray')

function(name='glIsVertexAttribEnabledAPPLE', enabled=False, function_type=FuncType.GET,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
    ],
)

function(name='glLGPUCopyImageSubDataNVX', enabled=False, function_type=FuncType.NONE,
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

function(name='glLGPUInterlockNVX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glLGPUNamedBufferSubDataNVX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='gpuMask', type='GLbitfield'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource'),
    ],
)

function(name='glLabelObjectEXT', enabled=True, function_type=FuncType.PARAM, interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='object', type='GLuint'),
        Argument(name='length', type='GLsizei'),
        Argument(name='label', type='const GLchar*', wrap_params='length, label'),
    ],
)

function(name='glLabelObjectWithResponsibleProcessAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='name', type='GLuint'),
        Argument(name='pid', type='GLint'),
    ],
)

function(name='glLightEnviSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glLightModelf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glLightModelfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glLightModeli', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glLightModeliv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname,params'),
    ],
)

function(name='glLightModelx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

function(name='glLightModelxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glLightModelx')

function(name='glLightModelxv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfixed*', wrap_type='CGLfixed::CSParamArray', wrap_params='pname,params'),
    ],
)

function(name='glLightModelxvOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glLightModelxv')

function(name='glLightf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glLightfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glLighti', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glLightiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glLightx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

function(name='glLightxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glLightx')

function(name='glLightxv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='light', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfixed*', wrap_type='CGLfixed::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glLightxvOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glLightxv')

function(name='glLineStipple', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLint'),
        Argument(name='pattern', type='GLushort'),
    ],
)

function(name='glLineWidth', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='width', type='GLfloat'),
    ],
)

function(name='glLineWidthx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='width', type='GLfixed'),
    ],
)

function(name='glLineWidthxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glLineWidthx')

function(name='glLinkProgram', enabled=True, function_type=FuncType.PARAM, recorder_wrap=True, run_wrap=True, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

function(name='glLinkProgramARB', enabled=True, function_type=FuncType.PARAM, recorder_wrap=True, run_wrap=True, state_track=True, inherit_from='glLinkProgram')

function(name='glListBase', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='base', type='GLuint'),
    ],
)

function(name='glListDrawCommandsStatesClientNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glListParameterfSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glListParameterfvSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

function(name='glListParameteriSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glListParameterivSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glLoadIdentity', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glLoadIdentityDeformationMapSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLbitfield'),
    ],
)

function(name='glLoadMatrixd', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLdouble*', wrap_params='16, m'),
    ],
)

function(name='glLoadMatrixf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLfloat*', wrap_params='16, m'),
    ],
)

function(name='glLoadMatrixx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLfixed*', wrap_params='16, m'),
    ],
)

function(name='glLoadMatrixxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glLoadMatrixx')

function(name='glLoadName', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='name', type='GLuint'),
    ],
)

function(name='glLoadPaletteFromModelViewMatrixOES', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glLoadProgramNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='id', type='GLuint'),
        Argument(name='len', type='GLsizei'),
        Argument(name='program', type='const GLubyte*'),
    ],
)

function(name='glLoadTransposeMatrixd', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLdouble*', wrap_params='16, m'),
    ],
)

function(name='glLoadTransposeMatrixdARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glLoadTransposeMatrixd')

function(name='glLoadTransposeMatrixf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLfloat*', wrap_params='16, m'),
    ],
)

function(name='glLoadTransposeMatrixfARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glLoadTransposeMatrixf')

function(name='glLoadTransposeMatrixxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLfixed*'),
    ],
)

function(name='glLockArraysEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLint'),
        Argument(name='count', type='GLsizei'),
    ],
)

function(name='glLogicOp', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='opcode', type='GLenum'),
    ],
)

function(name='glMakeBufferNonResidentNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

function(name='glMakeBufferResidentNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='access', type='GLenum'),
    ],
)

function(name='glMakeImageHandleNonResidentARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='handle', type='GLuint64', wrap_type='CGLTextureHandle'),
    ],
)

function(name='glMakeImageHandleNonResidentNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='handle', type='GLuint64'),
    ],
)

function(name='glMakeImageHandleResidentARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='handle', type='GLuint64', wrap_type='CGLTextureHandle'),
        Argument(name='access', type='GLenum'),
    ],
)

function(name='glMakeImageHandleResidentNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='handle', type='GLuint64'),
        Argument(name='access', type='GLenum'),
    ],
)

function(name='glMakeNamedBufferNonResidentNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

function(name='glMakeNamedBufferResidentNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='access', type='GLenum'),
    ],
)

function(name='glMakeTextureHandleNonResidentARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='handle', type='GLuint64', wrap_type='CGLTextureHandle'),
    ],
)

function(name='glMakeTextureHandleNonResidentNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='handle', type='GLuint64'),
    ],
)

function(name='glMakeTextureHandleResidentARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='handle', type='GLuint64', wrap_type='CGLTextureHandle'),
    ],
)

function(name='glMakeTextureHandleResidentNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='handle', type='GLuint64'),
    ],
)

function(name='glMap1d', enabled=True, function_type=FuncType.PARAM,
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

function(name='glMap1f', enabled=True, function_type=FuncType.PARAM,
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

function(name='glMap1xOES', enabled=False, function_type=FuncType.NONE,
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

function(name='glMap2d', enabled=True, function_type=FuncType.PARAM,
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

function(name='glMap2f', enabled=True, function_type=FuncType.PARAM,
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

function(name='glMap2xOES', enabled=False, function_type=FuncType.NONE,
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

function(name='glMapBuffer', enabled=True, function_type=FuncType.MAP, state_track=True, rec_condition='ConditionBufferES(_recorder)',
    return_value=ReturnValue(type='void*', wrap_type='CGLvoid_ptr'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='access', type='GLenum'),
    ],
)

function(name='glMapBufferARB', enabled=True, function_type=FuncType.MAP, inherit_from='glMapBuffer', state_track=True)

function(name='glMapBufferOES', enabled=True, function_type=FuncType.MAP, inherit_from='glMapBuffer', state_track=True)

function(name='glMapBufferRange', enabled=True, function_type=FuncType.MAP, state_track=True, rec_condition='ConditionBufferES(_recorder)', interceptor_exec_override=True,
    return_value=ReturnValue(type='void*', wrap_type='CGLvoid_ptr'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='length', type='GLsizeiptr'),
        Argument(name='access', type='GLbitfield', wrap_type='CMapAccess'),
    ],
)

function(name='glMapBufferRangeEXT', enabled=True, function_type=FuncType.MAP, interceptor_exec_override=True, inherit_from='glMapBufferRange')

function(name='glMapControlPointsNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glMapGrid1d', enabled=True, function_type=FuncType.MAP,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='un', type='GLint'),
        Argument(name='u1', type='GLdouble'),
        Argument(name='u2', type='GLdouble'),
    ],
)

function(name='glMapGrid1f', enabled=True, function_type=FuncType.MAP,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='un', type='GLint'),
        Argument(name='u1', type='GLfloat'),
        Argument(name='u2', type='GLfloat'),
    ],
)

function(name='glMapGrid1xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLint'),
        Argument(name='u1', type='GLfixed'),
        Argument(name='u2', type='GLfixed'),
    ],
)

function(name='glMapGrid2d', enabled=True, function_type=FuncType.MAP,
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

function(name='glMapGrid2f', enabled=True, function_type=FuncType.MAP,
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

function(name='glMapGrid2xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLint'),
        Argument(name='u1', type='GLfixed'),
        Argument(name='u2', type='GLfixed'),
        Argument(name='v1', type='GLfixed'),
        Argument(name='v2', type='GLfixed'),
    ],
)

function(name='glMapNamedBuffer', enabled=True, function_type=FuncType.MAP, state_track=True,
    return_value=ReturnValue(type='void*'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='access', type='GLenum'),
    ],
)

function(name='glMapNamedBufferEXT', enabled=True, function_type=FuncType.MAP, inherit_from='glMapNamedBuffer')

function(name='glMapNamedBufferRange', enabled=True, function_type=FuncType.MAP, state_track=True, interceptor_exec_override=True,
    return_value=ReturnValue(type='void*'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='length', type='GLsizeiptr'),
        Argument(name='access', type='GLbitfield', wrap_type='CMapAccess'),
    ],
)

function(name='glMapNamedBufferRangeEXT', enabled=True, function_type=FuncType.MAP, interceptor_exec_override=True, inherit_from='glMapNamedBufferRange')

function(name='glMapObjectBufferATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void*'),
    args=[
        Argument(name='buffer', type='GLuint'),
    ],
)

function(name='glMapParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

function(name='glMapParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glMapTexture2DINTEL', enabled=True, function_type=FuncType.PARAM, state_track=True, interceptor_exec_override=True,
    return_value=ReturnValue(type='void*', wrap_type='CGLMappedTexturePtr'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='access', type='GLbitfield', wrap_params='GL_MAP_WRITE_BIT | GL_MAP_READ_BIT'),
        Argument(name='stride', type='GLint*', wrap_type='COutArgument'),
        Argument(name='layout', type='GLenum*', wrap_type='COutArgument'),
    ],
)

function(name='glMapVertexAttrib1dAPPLE', enabled=False, function_type=FuncType.NONE,
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

function(name='glMapVertexAttrib1fAPPLE', enabled=False, function_type=FuncType.NONE,
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

function(name='glMapVertexAttrib2dAPPLE', enabled=False, function_type=FuncType.NONE,
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

function(name='glMapVertexAttrib2fAPPLE', enabled=False, function_type=FuncType.NONE,
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

function(name='glMaterialf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glMaterialfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glMateriali', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glMaterialiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glMaterialx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

function(name='glMaterialxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glMaterialx')

function(name='glMaterialxv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfixed*', wrap_type='CGLfixed::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glMaterialxvOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glMaterialxv')

function(name='glMatrixFrustumEXT', enabled=True, function_type=FuncType.PARAM,
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

function(name='glMatrixIndexPointerARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glMatrixIndexPointerOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glMatrixIndexPointerOESBounds', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const GLvoid*'),
        Argument(name='count', type='GLsizei'),
    ],
)

function(name='glMatrixIndexubvARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='indices', type='const GLubyte*'),
    ],
)

function(name='glMatrixIndexuivARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='indices', type='const GLuint*'),
    ],
)

function(name='glMatrixIndexusvARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='indices', type='const GLushort*'),
    ],
)

function(name='glMatrixLoad3x2fNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='matrixMode', type='GLenum'),
        Argument(name='m', type='const GLfloat*'),
    ],
)

function(name='glMatrixLoad3x3fNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='matrixMode', type='GLenum'),
        Argument(name='m', type='const GLfloat*'),
    ],
)

function(name='glMatrixLoadIdentityEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glMatrixLoadTranspose3x3fNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='matrixMode', type='GLenum'),
        Argument(name='m', type='const GLfloat*'),
    ],
)

function(name='glMatrixLoadTransposedEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='m', type='const GLdouble*'),
    ],
)

function(name='glMatrixLoadTransposefEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='m', type='const GLfloat*'),
    ],
)

function(name='glMatrixLoaddEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='m', type='const GLdouble*', wrap_params='16, m'),
    ],
)

function(name='glMatrixLoadfEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='m', type='const GLfloat*', wrap_params='16, m'),
    ],
)

function(name='glMatrixMode', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glMatrixMult3x2fNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='matrixMode', type='GLenum'),
        Argument(name='m', type='const GLfloat*'),
    ],
)

function(name='glMatrixMult3x3fNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='matrixMode', type='GLenum'),
        Argument(name='m', type='const GLfloat*'),
    ],
)

function(name='glMatrixMultTranspose3x3fNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='matrixMode', type='GLenum'),
        Argument(name='m', type='const GLfloat*'),
    ],
)

function(name='glMatrixMultTransposedEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='m', type='const GLdouble*'),
    ],
)

function(name='glMatrixMultTransposefEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='m', type='const GLfloat*'),
    ],
)

function(name='glMatrixMultdEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='m', type='const GLdouble*'),
    ],
)

function(name='glMatrixMultfEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='m', type='const GLfloat*'),
    ],
)

function(name='glMatrixOrthoEXT', enabled=True, function_type=FuncType.PARAM,
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

function(name='glMatrixPopEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glMatrixPushEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glMatrixRotatedEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='angle', type='GLdouble'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

function(name='glMatrixRotatefEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='angle', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

function(name='glMatrixScaledEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

function(name='glMatrixScalefEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

function(name='glMatrixTranslatedEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

function(name='glMatrixTranslatefEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

function(name='glMaxShaderCompilerThreadsARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='count', type='GLuint'),
    ],
)

function(name='glMaxShaderCompilerThreadsKHR', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='count', type='GLuint'),
    ],
)

function(name='glMemoryBarrier', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='barriers', type='GLbitfield'),
    ],
)

function(name='glMemoryBarrierByRegion', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='barriers', type='GLbitfield'),
    ],
)

function(name='glMemoryBarrierEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glMemoryBarrier')

function(name='glMemoryObjectParameterivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='memoryObject', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glMinSampleShading', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='value', type='GLfloat'),
    ],
)

function(name='glMinSampleShadingARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMinSampleShading')

function(name='glMinSampleShadingOES', enabled=False, function_type=FuncType.NONE, inherit_from='glMinSampleShading')

function(name='glMinmax', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='sink', type='GLboolean'),
    ],
)

function(name='glMinmaxEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glMinmax')

function(name='glMultMatrixd', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLdouble*', wrap_params='16, m'),
    ],
)

function(name='glMultMatrixf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLfloat*', wrap_params='16, m'),
    ],
)

function(name='glMultMatrixx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLfixed*', wrap_params='16, m'),
    ],
)

function(name='glMultMatrixxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultMatrixx')

function(name='glMultTransposeMatrixd', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLdouble*', wrap_params='16, m'),
    ],
)

function(name='glMultTransposeMatrixdARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultTransposeMatrixd')

function(name='glMultTransposeMatrixf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLfloat*', wrap_params='16, m'),
    ],
)

function(name='glMultTransposeMatrixfARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultTransposeMatrixf')

function(name='glMultTransposeMatrixxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='m', type='const GLfixed*'),
    ],
)

function(name='glMultiDrawArrays', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(first, count, drawcount)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='first', type='const GLint*', wrap_params='drawcount, first'),
        Argument(name='count', type='const GLsizei*', wrap_params='drawcount, count'),
        Argument(name='drawcount', type='GLsizei'),
    ],
)

function(name='glMultiDrawArraysEXT', enabled=True, function_type=FuncType.RENDER, inherit_from='glMultiDrawArrays')

function(name='glMultiDrawArraysIndirect', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientIndirectArraysUpdate(mode, indirect, drawcount, stride)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='indirect', type='const void*', wrap_type='CIndirectPtr'),
        Argument(name='drawcount', type='GLsizei'),
        Argument(name='stride', type='GLsizei'),
    ],
)

function(name='glMultiDrawArraysIndirectAMD', enabled=True, function_type=FuncType.RENDER, inherit_from='glMultiDrawArraysIndirect')

function(name='glMultiDrawArraysIndirectBindlessCountNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glMultiDrawArraysIndirectBindlessNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='indirect', type='const void*', wrap_type='CIndirectPtr'),
        Argument(name='drawCount', type='GLsizei'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='vertexBufferCount', type='GLint'),
    ],
)

function(name='glMultiDrawArraysIndirectCount', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='indirect', type='const void*', wrap_type='CIndirectPtr'),
        Argument(name='drawcount', type='GLintptr'),
        Argument(name='maxdrawcount', type='GLsizei'),
        Argument(name='stride', type='GLsizei'),
    ],
)

function(name='glMultiDrawArraysIndirectCountARB', enabled=False, function_type=FuncType.NONE, inherit_from='glMultiDrawArraysIndirectCount')

function(name='glMultiDrawArraysIndirectEXT', enabled=False, function_type=FuncType.RENDER, inherit_from='glMultiDrawArraysIndirect')

function(name='glMultiDrawElementArrayAPPLE', enabled=False, function_type=FuncType.NONE, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='first', type='const GLint*'),
        Argument(name='count', type='const GLsizei*'),
        Argument(name='primcount', type='GLsizei'),
    ],
)

function(name='glMultiDrawElements', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientArraysUpdate(type, count, indices, drawcount)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='count', type='const GLsizei*', wrap_params='drawcount, count'),
        Argument(name='type', type='GLenum'),
        Argument(name='indices', type='const void*const*', wrap_type='CDataPtrArray', wrap_params='GL_ELEMENT_ARRAY_BUFFER, indices, drawcount'),
        Argument(name='drawcount', type='GLsizei'),
    ],
)

function(name='glMultiDrawElementsBaseVertex', enabled=False, function_type=FuncType.NONE, exec_post_recorder_wrap=True,
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

function(name='glMultiDrawElementsBaseVertexEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glMultiDrawElementsBaseVertex')

function(name='glMultiDrawElementsBaseVertexOES', enabled=False, function_type=FuncType.NONE,
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

function(name='glMultiDrawElementsEXT', enabled=True, function_type=FuncType.RENDER, inherit_from='glMultiDrawElements')

function(name='glMultiDrawElementsIndirect', enabled=True, function_type=FuncType.RENDER, pre_token='CgitsClientIndirectArraysUpdate(mode, type, indirect, drawcount, stride)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='indirect', type='const void*', wrap_type='CIndirectPtr'),
        Argument(name='drawcount', type='GLsizei'),
        Argument(name='stride', type='GLsizei'),
    ],
)

function(name='glMultiDrawElementsIndirectAMD', enabled=True, function_type=FuncType.RENDER, inherit_from='glMultiDrawElementsIndirect')

function(name='glMultiDrawElementsIndirectBindlessCountNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glMultiDrawElementsIndirectBindlessNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glMultiDrawElementsIndirectCount', enabled=True, function_type=FuncType.RENDER,
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

function(name='glMultiDrawElementsIndirectCountARB', enabled=True, function_type=FuncType.RENDER, inherit_from='glMultiDrawElementsIndirectCount')

function(name='glMultiDrawElementsIndirectEXT', enabled=False, function_type=FuncType.RENDER, inherit_from='glMultiDrawElementsIndirect')

function(name='glMultiDrawRangeElementArrayAPPLE', enabled=False, function_type=FuncType.NONE, exec_post_recorder_wrap=True,
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

function(name='glMultiModeDrawArraysIBM', enabled=False, function_type=FuncType.NONE, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='const GLenum*'),
        Argument(name='first', type='const GLint*'),
        Argument(name='count', type='const GLsizei*'),
        Argument(name='primcount', type='GLsizei'),
        Argument(name='modestride', type='GLint'),
    ],
)

function(name='glMultiModeDrawElementsIBM', enabled=False, function_type=FuncType.NONE, exec_post_recorder_wrap=True,
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

function(name='glMultiTexBufferEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

function(name='glMultiTexCoord1bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='s', type='GLbyte'),
    ],
)

function(name='glMultiTexCoord1bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='coords', type='const GLbyte*'),
    ],
)

function(name='glMultiTexCoord1d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLdouble'),
    ],
)

function(name='glMultiTexCoord1dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord1d')

function(name='glMultiTexCoord1dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLdouble*', wrap_params='1, v'),
    ],
)

function(name='glMultiTexCoord1dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord1dv')

function(name='glMultiTexCoord1f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLfloat'),
    ],
)

function(name='glMultiTexCoord1fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord1f')

function(name='glMultiTexCoord1fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLfloat*', wrap_params='1, v'),
    ],
)

function(name='glMultiTexCoord1fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord1fv')

function(name='glMultiTexCoord1hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLhalfNV'),
    ],
)

function(name='glMultiTexCoord1hvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLhalfNV*'),
    ],
)

function(name='glMultiTexCoord1i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLint'),
    ],
)

function(name='glMultiTexCoord1iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord1i')

function(name='glMultiTexCoord1iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLint*', wrap_params='1, v'),
    ],
)

function(name='glMultiTexCoord1ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord1iv')

function(name='glMultiTexCoord1s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLshort'),
    ],
)

function(name='glMultiTexCoord1sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord1s')

function(name='glMultiTexCoord1sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLshort*', wrap_params='1, v'),
    ],
)

function(name='glMultiTexCoord1svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord1sv')

function(name='glMultiTexCoord1xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='s', type='GLfixed'),
    ],
)

function(name='glMultiTexCoord1xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='coords', type='const GLfixed*'),
    ],
)

function(name='glMultiTexCoord2bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='s', type='GLbyte'),
        Argument(name='t', type='GLbyte'),
    ],
)

function(name='glMultiTexCoord2bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='coords', type='const GLbyte*'),
    ],
)

function(name='glMultiTexCoord2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLdouble'),
        Argument(name='t', type='GLdouble'),
    ],
)

function(name='glMultiTexCoord2dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord2d')

function(name='glMultiTexCoord2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLdouble*', wrap_params='2, v'),
    ],
)

function(name='glMultiTexCoord2dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord2dv')

function(name='glMultiTexCoord2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
    ],
)

function(name='glMultiTexCoord2fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord2f')

function(name='glMultiTexCoord2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLfloat*', wrap_params='2, v'),
    ],
)

function(name='glMultiTexCoord2fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord2fv')

function(name='glMultiTexCoord2hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLhalfNV'),
        Argument(name='t', type='GLhalfNV'),
    ],
)

function(name='glMultiTexCoord2hvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLhalfNV*'),
    ],
)

function(name='glMultiTexCoord2i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLint'),
        Argument(name='t', type='GLint'),
    ],
)

function(name='glMultiTexCoord2iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord2i')

function(name='glMultiTexCoord2iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLint*', wrap_params='2, v'),
    ],
)

function(name='glMultiTexCoord2ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord2iv')

function(name='glMultiTexCoord2s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLshort'),
        Argument(name='t', type='GLshort'),
    ],
)

function(name='glMultiTexCoord2sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord2s')

function(name='glMultiTexCoord2sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLshort*', wrap_params='2, v'),
    ],
)

function(name='glMultiTexCoord2svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord2sv')

function(name='glMultiTexCoord2xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='s', type='GLfixed'),
        Argument(name='t', type='GLfixed'),
    ],
)

function(name='glMultiTexCoord2xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='coords', type='const GLfixed*'),
    ],
)

function(name='glMultiTexCoord3bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='s', type='GLbyte'),
        Argument(name='t', type='GLbyte'),
        Argument(name='r', type='GLbyte'),
    ],
)

function(name='glMultiTexCoord3bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='coords', type='const GLbyte*'),
    ],
)

function(name='glMultiTexCoord3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLdouble'),
        Argument(name='t', type='GLdouble'),
        Argument(name='r', type='GLdouble'),
    ],
)

function(name='glMultiTexCoord3dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord3d')

function(name='glMultiTexCoord3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

function(name='glMultiTexCoord3dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord3dv')

function(name='glMultiTexCoord3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='r', type='GLfloat'),
    ],
)

function(name='glMultiTexCoord3fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord3f')

function(name='glMultiTexCoord3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

function(name='glMultiTexCoord3fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord3fv')

function(name='glMultiTexCoord3hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLhalfNV'),
        Argument(name='t', type='GLhalfNV'),
        Argument(name='r', type='GLhalfNV'),
    ],
)

function(name='glMultiTexCoord3hvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLhalfNV*'),
    ],
)

function(name='glMultiTexCoord3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLint'),
        Argument(name='t', type='GLint'),
        Argument(name='r', type='GLint'),
    ],
)

function(name='glMultiTexCoord3iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord3i')

function(name='glMultiTexCoord3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

function(name='glMultiTexCoord3ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord3iv')

function(name='glMultiTexCoord3s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLshort'),
        Argument(name='t', type='GLshort'),
        Argument(name='r', type='GLshort'),
    ],
)

function(name='glMultiTexCoord3sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord3s')

function(name='glMultiTexCoord3sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

function(name='glMultiTexCoord3svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord3sv')

function(name='glMultiTexCoord3xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='s', type='GLfixed'),
        Argument(name='t', type='GLfixed'),
        Argument(name='r', type='GLfixed'),
    ],
)

function(name='glMultiTexCoord3xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='coords', type='const GLfixed*'),
    ],
)

function(name='glMultiTexCoord4bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='s', type='GLbyte'),
        Argument(name='t', type='GLbyte'),
        Argument(name='r', type='GLbyte'),
        Argument(name='q', type='GLbyte'),
    ],
)

function(name='glMultiTexCoord4bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='coords', type='const GLbyte*'),
    ],
)

function(name='glMultiTexCoord4d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLdouble'),
        Argument(name='t', type='GLdouble'),
        Argument(name='r', type='GLdouble'),
        Argument(name='q', type='GLdouble'),
    ],
)

function(name='glMultiTexCoord4dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord4d')

function(name='glMultiTexCoord4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLdouble*', wrap_params='4, v'),
    ],
)

function(name='glMultiTexCoord4dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord4dv')

function(name='glMultiTexCoord4f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='r', type='GLfloat'),
        Argument(name='q', type='GLfloat'),
    ],
)

function(name='glMultiTexCoord4fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord4f')

function(name='glMultiTexCoord4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

function(name='glMultiTexCoord4fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord4fv')

function(name='glMultiTexCoord4hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLhalfNV'),
        Argument(name='t', type='GLhalfNV'),
        Argument(name='r', type='GLhalfNV'),
        Argument(name='q', type='GLhalfNV'),
    ],
)

function(name='glMultiTexCoord4hvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLhalfNV*'),
    ],
)

function(name='glMultiTexCoord4i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLint'),
        Argument(name='t', type='GLint'),
        Argument(name='r', type='GLint'),
        Argument(name='q', type='GLint'),
    ],
)

function(name='glMultiTexCoord4iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord4i')

function(name='glMultiTexCoord4iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

function(name='glMultiTexCoord4ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord4iv')

function(name='glMultiTexCoord4s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='s', type='GLshort'),
        Argument(name='t', type='GLshort'),
        Argument(name='r', type='GLshort'),
        Argument(name='q', type='GLshort'),
    ],
)

function(name='glMultiTexCoord4sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord4s')

function(name='glMultiTexCoord4sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='v', type='const GLshort*', wrap_params='4, v'),
    ],
)

function(name='glMultiTexCoord4svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord4sv')

function(name='glMultiTexCoord4x', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='s', type='GLfixed'),
        Argument(name='t', type='GLfixed'),
        Argument(name='r', type='GLfixed'),
        Argument(name='q', type='GLfixed'),
    ],
)

function(name='glMultiTexCoord4xOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glMultiTexCoord4x')

function(name='glMultiTexCoord4xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='coords', type='const GLfixed*'),
    ],
)

function(name='glMultiTexCoordP1ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='GLuint'),
    ],
)

function(name='glMultiTexCoordP1uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='const GLuint*', wrap_params='1, coords'),
    ],
)

function(name='glMultiTexCoordP2ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='GLuint'),
    ],
)

function(name='glMultiTexCoordP2uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='const GLuint*', wrap_params='2, coords'),
    ],
)

function(name='glMultiTexCoordP3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='GLuint'),
    ],
)

function(name='glMultiTexCoordP3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='const GLuint*', wrap_params='3, coords'),
    ],
)

function(name='glMultiTexCoordP4ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='GLuint'),
    ],
)

function(name='glMultiTexCoordP4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLenum'),
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='const GLuint*', wrap_params='4, coords'),
    ],
)

function(name='glMultiTexCoordPointerEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

function(name='glMultiTexEnvfEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glMultiTexEnvfvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glMultiTexEnviEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glMultiTexEnvivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glMultiTexGendEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLdouble'),
    ],
)

function(name='glMultiTexGendvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLdouble*'),
    ],
)

function(name='glMultiTexGenfEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glMultiTexGenfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

function(name='glMultiTexGeniEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glMultiTexGenivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glMultiTexImage1DEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glMultiTexImage2DEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glMultiTexImage3DEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glMultiTexParameterIivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glMultiTexParameterIuivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLuint*', wrap_type='CGLuint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glMultiTexParameterfEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glMultiTexParameterfvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glMultiTexParameteriEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glMultiTexParameterivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glMultiTexRenderbufferEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texunit', type='GLenum'),
        Argument(name='target', type='GLenum'),
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
    ],
)

function(name='glMultiTexSubImage1DEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glMultiTexSubImage2DEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glMultiTexSubImage3DEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glMulticastBarrierNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glMulticastBlitFramebufferNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glMulticastBufferSubDataNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='gpuMask', type='GLbitfield'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource'),
    ],
)

function(name='glMulticastCopyBufferSubDataNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glMulticastCopyImageSubDataNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glMulticastFramebufferSampleLocationsfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='gpu', type='GLuint'),
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='start', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glMulticastGetQueryObjecti64vNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='gpu', type='GLuint'),
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint64*'),
    ],
)

function(name='glMulticastGetQueryObjectivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='gpu', type='GLuint'),
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLint*'),
    ],
)

function(name='glMulticastGetQueryObjectui64vNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='gpu', type='GLuint'),
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint64*'),
    ],
)

function(name='glMulticastGetQueryObjectuivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='gpu', type='GLuint'),
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='GLuint*'),
    ],
)

function(name='glMulticastWaitSyncNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='signalGpu', type='GLuint'),
        Argument(name='waitGpuMask', type='GLbitfield'),
    ],
)

function(name='glNamedBufferData', enabled=True, function_type=FuncType.RESOURCE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, size'),
        Argument(name='usage', type='GLenum'),
    ],
)

function(name='glNamedBufferDataEXT', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glNamedBufferData')

function(name='glNamedBufferPageCommitmentARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='commit', type='GLboolean'),
    ],
)

function(name='glNamedBufferPageCommitmentEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='commit', type='GLboolean'),
    ],
)

function(name='glNamedBufferStorage', enabled=True, function_type=FuncType.RESOURCE, state_track=True, interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, size'),
        Argument(name='flags', type='GLbitfield', wrap_type='CBufferStorageFlags'),
    ],
)

function(name='glNamedBufferStorageEXT', enabled=True, function_type=FuncType.RESOURCE, interceptor_exec_override=True, inherit_from='glNamedBufferStorage')

function(name='glNamedBufferStorageExternalEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='clientBuffer', type='GLeglClientBufferEXT'),
        Argument(name='flags', type='GLbitfield'),
    ],
)

function(name='glNamedBufferStorageMemEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='memory', type='GLuint'),
        Argument(name='offset', type='GLuint64'),
    ],
)

function(name='glNamedBufferSubData', enabled=True, function_type=FuncType.RESOURCE, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
        Argument(name='data', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, data, size'),
    ],
)

function(name='glNamedBufferSubDataEXT', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glNamedBufferSubData')

function(name='glNamedCopyBufferSubDataEXT', enabled=True, function_type=FuncType.COPY,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='readBuffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='writeBuffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='readOffset', type='GLintptr'),
        Argument(name='writeOffset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
    ],
)

function(name='glNamedFramebufferDrawBuffer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='buf', type='GLenum'),
    ],
)

function(name='glNamedFramebufferDrawBuffers', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='n', type='GLsizei'),
        Argument(name='bufs', type='const GLenum*', wrap_params='n, bufs'),
    ],
)

function(name='glNamedFramebufferParameteri', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glNamedFramebufferParameteriEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glNamedFramebufferParameteri')

function(name='glNamedFramebufferReadBuffer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint'),
        Argument(name='src', type='GLenum'),
    ],
)

function(name='glNamedFramebufferRenderbuffer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='renderbuffertarget', type='GLenum'),
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
    ],
)

function(name='glNamedFramebufferRenderbufferEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glNamedFramebufferRenderbuffer')

function(name='glNamedFramebufferSampleLocationsfvARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='start', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glNamedFramebufferSampleLocationsfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='start', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glNamedFramebufferSamplePositionsfvAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='numsamples', type='GLuint'),
        Argument(name='pixelindex', type='GLuint'),
        Argument(name='values', type='const GLfloat*'),
    ],
)

function(name='glNamedFramebufferTexture', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
    ],
)

function(name='glNamedFramebufferTexture1DEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='textarget', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
    ],
)

function(name='glNamedFramebufferTexture2DEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='textarget', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
    ],
)

function(name='glNamedFramebufferTexture3DEXT', enabled=True, function_type=FuncType.PARAM,
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

function(name='glNamedFramebufferTextureEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glNamedFramebufferTexture')

function(name='glNamedFramebufferTextureFaceEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='face', type='GLenum'),
    ],
)

function(name='glNamedFramebufferTextureLayer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='framebuffer', type='GLuint', wrap_type='CGLFramebuffer'),
        Argument(name='attachment', type='GLenum'),
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
        Argument(name='layer', type='GLint'),
    ],
)

function(name='glNamedFramebufferTextureLayerEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glNamedFramebufferTextureLayer')

function(name='glNamedProgramLocalParameter4dEXT', enabled=True, function_type=FuncType.PARAM,
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

function(name='glNamedProgramLocalParameter4dvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLdouble*', wrap_params='4, params'),
    ],
)

function(name='glNamedProgramLocalParameter4fEXT', enabled=True, function_type=FuncType.PARAM,
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

function(name='glNamedProgramLocalParameter4fvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLfloat*', wrap_params='4, params'),
    ],
)

function(name='glNamedProgramLocalParameterI4iEXT', enabled=True, function_type=FuncType.PARAM,
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

function(name='glNamedProgramLocalParameterI4ivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLint*', wrap_params='4, params'),
    ],
)

function(name='glNamedProgramLocalParameterI4uiEXT', enabled=True, function_type=FuncType.PARAM,
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

function(name='glNamedProgramLocalParameterI4uivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLuint*', wrap_params='4, params'),
    ],
)

function(name='glNamedProgramLocalParameters4fvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLfloat*', wrap_params='count*4, params'),
    ],
)

function(name='glNamedProgramLocalParametersI4ivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLint*', wrap_params='count*4, params'),
    ],
)

function(name='glNamedProgramLocalParametersI4uivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLuint*', wrap_params='count*4, params'),
    ],
)

function(name='glNamedProgramStringEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='target', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='len', type='GLsizei'),
        Argument(name='string', type='const void*'),
    ],
)

function(name='glNamedRenderbufferStorage', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

function(name='glNamedRenderbufferStorageEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glNamedRenderbufferStorage',
    args=[
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbufferEXT'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

function(name='glNamedRenderbufferStorageMultisample', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

function(name='glNamedRenderbufferStorageMultisampleCoverageEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glNamedRenderbufferStorageMultisampleEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glNamedRenderbufferStorageMultisample',
    args=[
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbufferEXT'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

function(name='glNamedStringARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='namelen', type='GLint'),
        Argument(name='name', type='const GLchar*', wrap_params='name, \'\\0\', 1'),
        Argument(name='stringlen', type='GLint'),
        Argument(name='string', type='const GLchar*'),
    ],
)

function(name='glNewList', enabled=True, function_type=FuncType.PARAM, recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='list', type='GLuint'),
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glNewObjectBufferATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLuint'),
    args=[
        Argument(name='size', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
        Argument(name='usage', type='GLenum'),
    ],
)

function(name='glNormal3b', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='nx', type='GLbyte'),
        Argument(name='ny', type='GLbyte'),
        Argument(name='nz', type='GLbyte'),
    ],
)

function(name='glNormal3bv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLbyte*', wrap_params='3, v'),
    ],
)

function(name='glNormal3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='nx', type='GLdouble'),
        Argument(name='ny', type='GLdouble'),
        Argument(name='nz', type='GLdouble'),
    ],
)

function(name='glNormal3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

function(name='glNormal3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='nx', type='GLfloat'),
        Argument(name='ny', type='GLfloat'),
        Argument(name='nz', type='GLfloat'),
    ],
)

function(name='glNormal3fVertex3fSUN', enabled=True, function_type=FuncType.PARAM,
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

function(name='glNormal3fVertex3fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='const GLfloat*', wrap_params='3, v'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

function(name='glNormal3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

function(name='glNormal3hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='nx', type='GLhalfNV'),
        Argument(name='ny', type='GLhalfNV'),
        Argument(name='nz', type='GLhalfNV'),
    ],
)

function(name='glNormal3hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='3, v'),
    ],
)

function(name='glNormal3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='nx', type='GLint'),
        Argument(name='ny', type='GLint'),
        Argument(name='nz', type='GLint'),
    ],
)

function(name='glNormal3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

function(name='glNormal3s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='nx', type='GLshort'),
        Argument(name='ny', type='GLshort'),
        Argument(name='nz', type='GLshort'),
    ],
)

function(name='glNormal3sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

function(name='glNormal3x', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='nx', type='GLfixed'),
        Argument(name='ny', type='GLfixed'),
        Argument(name='nz', type='GLfixed'),
    ],
)

function(name='glNormal3xOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glNormal3x')

function(name='glNormal3xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

function(name='glNormalFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
    ],
)

function(name='glNormalP3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='GLuint'),
    ],
)

function(name='glNormalP3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='const GLuint*', wrap_params='3, coords'),
    ],
)

function(name='glNormalPointer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

function(name='glNormalPointerBounds', enabled=True, function_type=FuncType.PARAM, run_wrap=True, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const GLvoid*', wrap_type='CAttribPtr'),
        Argument(name='count', type='GLsizei'),
    ],
)

function(name='glNormalPointerEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='count', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

function(name='glNormalPointerListIBM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLint'),
        Argument(name='pointer', type='const void**'),
        Argument(name='ptrstride', type='GLint'),
    ],
)

function(name='glNormalPointervINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='pointer', type='const void**'),
    ],
)

function(name='glNormalStream3bATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='nx', type='GLbyte'),
        Argument(name='ny', type='GLbyte'),
        Argument(name='nz', type='GLbyte'),
    ],
)

function(name='glNormalStream3bvATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLbyte*'),
    ],
)

function(name='glNormalStream3dATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='nx', type='GLdouble'),
        Argument(name='ny', type='GLdouble'),
        Argument(name='nz', type='GLdouble'),
    ],
)

function(name='glNormalStream3dvATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLdouble*'),
    ],
)

function(name='glNormalStream3fATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='nx', type='GLfloat'),
        Argument(name='ny', type='GLfloat'),
        Argument(name='nz', type='GLfloat'),
    ],
)

function(name='glNormalStream3fvATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLfloat*'),
    ],
)

function(name='glNormalStream3iATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='nx', type='GLint'),
        Argument(name='ny', type='GLint'),
        Argument(name='nz', type='GLint'),
    ],
)

function(name='glNormalStream3ivATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLint*'),
    ],
)

function(name='glNormalStream3sATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='nx', type='GLshort'),
        Argument(name='ny', type='GLshort'),
        Argument(name='nz', type='GLshort'),
    ],
)

function(name='glNormalStream3svATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLshort*'),
    ],
)

function(name='glObjectLabel', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='identifier', type='GLenum'),
        Argument(name='name', type='GLuint'),
        Argument(name='length', type='GLsizei'),
        Argument(name='label', type='const GLchar*', wrap_params='label, \'\\0\', 1'),
    ],
)

function(name='glObjectLabelKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glObjectLabel')

function(name='glObjectPtrLabel', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='ptr', type='const void*'),
        Argument(name='length', type='GLsizei'),
        Argument(name='label', type='const GLchar*'),
    ],
)

function(name='glObjectPtrLabelKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glObjectPtrLabel')

function(name='glObjectPurgeableAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='objectType', type='GLenum'),
        Argument(name='name', type='GLuint'),
        Argument(name='option', type='GLenum'),
    ],
)

function(name='glObjectUnpurgeableAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='objectType', type='GLenum'),
        Argument(name='name', type='GLuint'),
        Argument(name='option', type='GLenum'),
    ],
)

function(name='glOrtho', enabled=True, function_type=FuncType.PARAM,
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

function(name='glOrthof', enabled=True, function_type=FuncType.PARAM,
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

function(name='glOrthofOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glOrthof')

function(name='glOrthox', enabled=True, function_type=FuncType.PARAM,
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

function(name='glOrthoxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glOrthox')

function(name='glPNTrianglesfATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glPNTrianglesiATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glPassTexCoordATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='dst', type='GLuint'),
        Argument(name='coord', type='GLuint'),
        Argument(name='swizzle', type='GLenum'),
    ],
)

function(name='glPassThrough', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='token', type='GLfloat'),
    ],
)

function(name='glPassThroughxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='token', type='GLfixed'),
    ],
)

# FIXME: Argument `values` had a `wrapParam='CGLfloat::CSParamArray'`, which was a typo of wrapType.
# FIXME: Should we add that wrap_type or not?
function(name='glPatchParameterfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='values', type='const GLfloat*', wrap_params='GetPatchParameterValuesCount(pname), values'),
    ],
)

function(name='glPatchParameterfvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glPatchParameterfv')

function(name='glPatchParameteri', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLint'),
    ],
)

function(name='glPatchParameteriARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glPatchParameteri')

function(name='glPatchParameteriEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glPatchParameteri')

function(name='glPatchParameteriOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glPatchParameteri')

function(name='glPathColorGenNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='color', type='GLenum'),
        Argument(name='genMode', type='GLenum'),
        Argument(name='colorFormat', type='GLenum'),
        Argument(name='coeffs', type='const GLfloat*'),
    ],
)

function(name='glPathCommandsNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glPathCoordsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='numCoords', type='GLsizei'),
        Argument(name='coordType', type='GLenum'),
        Argument(name='coords', type='const void*'),
    ],
)

function(name='glPathCoverDepthFuncNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='func', type='GLenum'),
    ],
)

function(name='glPathDashArrayNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='dashCount', type='GLsizei'),
        Argument(name='dashArray', type='const GLfloat*'),
    ],
)

function(name='glPathFogGenNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='genMode', type='GLenum'),
    ],
)

function(name='glPathGlyphIndexArrayNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glPathGlyphIndexRangeNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glPathGlyphRangeNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glPathGlyphsNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glPathMemoryGlyphIndexArrayNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glPathParameterfNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLfloat'),
    ],
)

function(name='glPathParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='const GLfloat*'),
    ],
)

function(name='glPathParameteriNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLint'),
    ],
)

function(name='glPathParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='const GLint*'),
    ],
)

function(name='glPathStencilDepthOffsetNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLfloat'),
        Argument(name='units', type='GLfloat'),
    ],
)

function(name='glPathStencilFuncNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='func', type='GLenum'),
        Argument(name='ref', type='GLint'),
        Argument(name='mask', type='GLuint'),
    ],
)

function(name='glPathStringNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='format', type='GLenum'),
        Argument(name='length', type='GLsizei'),
        Argument(name='pathString', type='const void*'),
    ],
)

function(name='glPathSubCommandsNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glPathSubCoordsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='coordStart', type='GLsizei'),
        Argument(name='numCoords', type='GLsizei'),
        Argument(name='coordType', type='GLenum'),
        Argument(name='coords', type='const void*'),
    ],
)

function(name='glPathTexGenNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texCoordSet', type='GLenum'),
        Argument(name='genMode', type='GLenum'),
        Argument(name='components', type='GLint'),
        Argument(name='coeffs', type='const GLfloat*'),
    ],
)

function(name='glPauseTransformFeedback', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glPauseTransformFeedbackNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glPauseTransformFeedback')

function(name='glPixelDataRangeNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='length', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glPixelMapfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='mapsize', type='GLsizei'),
        Argument(name='values', type='const GLfloat*', wrap_params='mapsize, values'),
    ],
)

function(name='glPixelMapuiv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='mapsize', type='GLsizei'),
        Argument(name='values', type='const GLuint*'),
    ],
)

function(name='glPixelMapusv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='mapsize', type='GLsizei'),
        Argument(name='values', type='const GLushort*'),
    ],
)

function(name='glPixelMapx', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='map', type='GLenum'),
        Argument(name='size', type='GLint'),
        Argument(name='values', type='const GLfixed*'),
    ],
)

function(name='glPixelStoref', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glPixelStorei', enabled=True, function_type=FuncType.PARAM, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glPixelStorex', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

function(name='glPixelTexGenParameterfSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glPixelTexGenParameterfvSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

function(name='glPixelTexGenParameteriSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glPixelTexGenParameterivSGIS', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glPixelTexGenSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glPixelTransferf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glPixelTransferi', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glPixelTransferxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

function(name='glPixelTransformParameterfEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glPixelTransformParameterfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

function(name='glPixelTransformParameteriEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glPixelTransformParameterivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glPixelZoom', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='xfactor', type='GLfloat'),
        Argument(name='yfactor', type='GLfloat'),
    ],
)

function(name='glPixelZoomxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='xfactor', type='GLfixed'),
        Argument(name='yfactor', type='GLfixed'),
    ],
)

function(name='glPointAlongPathNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glPointParameterf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glPointParameterfARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameterf')

function(name='glPointParameterfEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameterf')

function(name='glPointParameterfSGIS', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameterf')

function(name='glPointParameterfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glPointParameterfvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameterfv')

function(name='glPointParameterfvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameterfv')

function(name='glPointParameterfvSGIS', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameterfv')

function(name='glPointParameteri', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glPointParameteriNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameteri')

function(name='glPointParameteriv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glPointParameterivNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameteriv')

function(name='glPointParameterx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

function(name='glPointParameterxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameterx')

function(name='glPointParameterxv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfixed*', wrap_type='CGLfixed::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glPointParameterxvOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointParameterxv')

function(name='glPointSize', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLfloat'),
    ],
)

function(name='glPointSizePointerAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const GLvoid*'),
    ],
)

function(name='glPointSizePointerOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glPointSizePointerOESBounds', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const GLvoid*'),
        Argument(name='count', type='GLsizei'),
    ],
)

function(name='glPointSizex', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLfixed'),
    ],
)

function(name='glPointSizexOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glPointSizex')

function(name='glPollAsyncSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='markerp', type='GLuint*'),
    ],
)

function(name='glPollInstrumentsSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='marker_p', type='GLint*'),
    ],
)

function(name='glPolygonMode', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glPolygonModeNV', enabled=False, function_type=FuncType.NONE, inherit_from='glPolygonMode')

function(name='glPolygonOffset', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLfloat'),
        Argument(name='units', type='GLfloat'),
    ],
)

function(name='glPolygonOffsetClamp', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLfloat'),
        Argument(name='units', type='GLfloat'),
        Argument(name='clamp', type='GLfloat'),
    ],
)

function(name='glPolygonOffsetClampEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glPolygonOffsetClamp')

function(name='glPolygonOffsetEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLfloat'),
        Argument(name='bias', type='GLfloat'),
    ],
)

function(name='glPolygonOffsetx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLfixed'),
        Argument(name='units', type='GLfixed'),
    ],
)

function(name='glPolygonOffsetxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glPolygonOffsetx')

function(name='glPolygonStipple', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='const GLubyte*', wrap_params='32 * 4, mask'),
    ],
)

function(name='glPopAttrib', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glPopClientAttrib', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glPopDebugGroup', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glPopDebugGroupKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glPopDebugGroup')

function(name='glPopGroupMarkerEXT', enabled=True, function_type=FuncType.PARAM, interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glPopMatrix', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glPopName', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glPresentFrameDualFillNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glPresentFrameKeyedNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glPrimitiveBoundingBox', enabled=False, function_type=FuncType.NONE,
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

function(name='glPrimitiveBoundingBoxARB', enabled=False, function_type=FuncType.NONE, inherit_from='glPrimitiveBoundingBox')

function(name='glPrimitiveBoundingBoxEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glPrimitiveBoundingBox')

function(name='glPrimitiveBoundingBoxOES', enabled=False, function_type=FuncType.NONE, inherit_from='glPrimitiveBoundingBox')

function(name='glPrimitiveRestartIndex', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
    ],
)

function(name='glPrimitiveRestartIndexNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glPrimitiveRestartIndex')

function(name='glPrimitiveRestartNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glPrioritizeTextures', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='textures', type='const GLuint*'),
        Argument(name='priorities', type='const GLfloat*'),
    ],
)

function(name='glPrioritizeTexturesEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glPrioritizeTextures')

function(name='glPrioritizeTexturesxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='textures', type='const GLuint*'),
        Argument(name='priorities', type='const GLfixed*'),
    ],
)

function(name='glProgramBinary', enabled=True, function_type=FuncType.RESOURCE, interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='binaryFormat', type='GLenum'),
        Argument(name='binary', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, binary, length'),
        Argument(name='length', type='GLsizei'),
    ],
)

function(name='glProgramBinaryOES', enabled=True, function_type=FuncType.RESOURCE, interceptor_exec_override=True, inherit_from='glProgramBinary')

function(name='glProgramBufferParametersIivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='bindingIndex', type='GLuint'),
        Argument(name='wordIndex', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glProgramBufferParametersIuivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='bindingIndex', type='GLuint'),
        Argument(name='wordIndex', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLuint*'),
    ],
)

function(name='glProgramBufferParametersfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='bindingIndex', type='GLuint'),
        Argument(name='wordIndex', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

function(name='glProgramEnvParameter4dARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
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

function(name='glProgramEnvParameter4dvARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLdouble*', wrap_params='4, params'),
    ],
)

function(name='glProgramEnvParameter4fARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
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

function(name='glProgramEnvParameter4fvARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLfloat*', wrap_params='4, params'),
    ],
)

function(name='glProgramEnvParameterI4iNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glProgramEnvParameterI4ivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glProgramEnvParameterI4uiNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glProgramEnvParameterI4uivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLuint*'),
    ],
)

function(name='glProgramEnvParameters4fvEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLfloat*', wrap_params='4 * count, params'),
    ],
)

function(name='glProgramEnvParametersI4ivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glProgramEnvParametersI4uivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLuint*'),
    ],
)

function(name='glProgramLocalParameter4dARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
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

function(name='glProgramLocalParameter4dvARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLdouble*', wrap_params='4, params'),
    ],
)

function(name='glProgramLocalParameter4fARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
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

function(name='glProgramLocalParameter4fvARB', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLfloat*', wrap_params='4, params'),
    ],
)

function(name='glProgramLocalParameterI4iNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glProgramLocalParameterI4ivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glProgramLocalParameterI4uiNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glProgramLocalParameterI4uivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='params', type='const GLuint*'),
    ],
)

function(name='glProgramLocalParameters4fvEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLfloat*', wrap_params='count * 4, params'),
    ],
)

function(name='glProgramLocalParametersI4ivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glProgramLocalParametersI4uivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLuint*'),
    ],
)

function(name='glProgramNamedParameter4dNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glProgramNamedParameter4dvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='len', type='GLsizei'),
        Argument(name='name', type='const GLubyte*'),
        Argument(name='v', type='const GLdouble*'),
    ],
)

function(name='glProgramNamedParameter4fNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glProgramNamedParameter4fvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='len', type='GLsizei'),
        Argument(name='name', type='const GLubyte*'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glProgramParameter4dNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glProgramParameter4dvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLdouble*'),
    ],
)

function(name='glProgramParameter4fNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glProgramParameter4fvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glProgramParameteri', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='pname', type='GLenum'),
        Argument(name='value', type='GLint'),
    ],
)

function(name='glProgramParameteriARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramParameteri')

function(name='glProgramParameteriEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramParameteri')

function(name='glProgramParameters4dvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLdouble*', wrap_params='count*4, v'),
    ],
)

function(name='glProgramParameters4fvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*', wrap_params='count*4, v'),
    ],
)

function(name='glProgramPathFragmentInputGenNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='genMode', type='GLenum'),
        Argument(name='components', type='GLint'),
        Argument(name='coeffs', type='const GLfloat*'),
    ],
)

function(name='glProgramStringARB', enabled=True, function_type=FuncType.PARAM, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='format', type='GLenum'),
        Argument(name='len', type='GLsizei'),
        Argument(name='string', type='const void*', wrap_type='CShaderSource', wrap_params='(const char *)string, len, CShaderSource::PROGRAM_STRING, target, format'),
    ],
)

function(name='glProgramSubroutineParametersuivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='count', type='GLsizei'),
        Argument(name='params', type='const GLuint*'),
    ],
)

function(name='glProgramUniform1d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLdouble'),
    ],
)

function(name='glProgramUniform1dEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform1d')

function(name='glProgramUniform1dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLdouble*', wrap_params='count, value'),
    ],
)

function(name='glProgramUniform1dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform1dv')

function(name='glProgramUniform1f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLfloat'),
    ],
)

function(name='glProgramUniform1fEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform1f')

function(name='glProgramUniform1fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLfloat*', wrap_params='count, value'),
    ],
)

function(name='glProgramUniform1fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform1fv')

function(name='glProgramUniform1i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLint'),
    ],
)

function(name='glProgramUniform1i64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLint64'),
    ],
)

function(name='glProgramUniform1i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLint64EXT'),
    ],
)

function(name='glProgramUniform1i64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64*', wrap_params='count, value'),
    ],
)

function(name='glProgramUniform1i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64EXT*', wrap_params='count, value'),
    ],
)

function(name='glProgramUniform1iEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform1i')

function(name='glProgramUniform1iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint*', wrap_params='count, value'),
    ],
)

function(name='glProgramUniform1ivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform1iv')

function(name='glProgramUniform1ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLuint'),
    ],
)

function(name='glProgramUniform1ui64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLuint64'),
    ],
)

function(name='glProgramUniform1ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLuint64EXT'),
    ],
)

function(name='glProgramUniform1ui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_params='count, value'),
    ],
)

function(name='glProgramUniform1ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count, value'),
    ],
)

function(name='glProgramUniform1uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform1ui')

function(name='glProgramUniform1uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint*', wrap_params='count, value'),
    ],
)

function(name='glProgramUniform1uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform1uiv')

function(name='glProgramUniform2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLdouble'),
        Argument(name='v1', type='GLdouble'),
    ],
)

function(name='glProgramUniform2dEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform2d')

function(name='glProgramUniform2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 2, value'),
    ],
)

function(name='glProgramUniform2dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform2dv')

function(name='glProgramUniform2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLfloat'),
        Argument(name='v1', type='GLfloat'),
    ],
)

function(name='glProgramUniform2fEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform2f')

function(name='glProgramUniform2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 2, value'),
    ],
)

function(name='glProgramUniform2fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform2fv')

function(name='glProgramUniform2i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLint'),
        Argument(name='v1', type='GLint'),
    ],
)

function(name='glProgramUniform2i64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLint64'),
        Argument(name='y', type='GLint64'),
    ],
)

function(name='glProgramUniform2i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLint64EXT'),
        Argument(name='y', type='GLint64EXT'),
    ],
)

function(name='glProgramUniform2i64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64*', wrap_params='count * 2, value'),
    ],
)

function(name='glProgramUniform2i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64EXT*', wrap_params='count * 2, value'),
    ],
)

function(name='glProgramUniform2iEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform2i')

function(name='glProgramUniform2iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint*', wrap_params='count * 2, value'),
    ],
)

function(name='glProgramUniform2ivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform2iv')

function(name='glProgramUniform2ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLuint'),
        Argument(name='v1', type='GLuint'),
    ],
)

function(name='glProgramUniform2ui64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLuint64'),
        Argument(name='y', type='GLuint64'),
    ],
)

function(name='glProgramUniform2ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLuint64EXT'),
        Argument(name='y', type='GLuint64EXT'),
    ],
)

function(name='glProgramUniform2ui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_params='count * 2, value'),
    ],
)

function(name='glProgramUniform2ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count * 2, value'),
    ],
)

function(name='glProgramUniform2uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform2ui')

function(name='glProgramUniform2uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint*', wrap_params='count * 2, value'),
    ],
)

function(name='glProgramUniform2uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform2uiv')

function(name='glProgramUniform3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLdouble'),
        Argument(name='v1', type='GLdouble'),
        Argument(name='v2', type='GLdouble'),
    ],
)

function(name='glProgramUniform3dEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform3d')

function(name='glProgramUniform3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 3, value'),
    ],
)

function(name='glProgramUniform3dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform3dv')

function(name='glProgramUniform3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLfloat'),
        Argument(name='v1', type='GLfloat'),
        Argument(name='v2', type='GLfloat'),
    ],
)

function(name='glProgramUniform3fEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform3f')

function(name='glProgramUniform3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 3, value'),
    ],
)

function(name='glProgramUniform3fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform3fv')

function(name='glProgramUniform3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLint'),
        Argument(name='v1', type='GLint'),
        Argument(name='v2', type='GLint'),
    ],
)

function(name='glProgramUniform3i64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLint64'),
        Argument(name='y', type='GLint64'),
        Argument(name='z', type='GLint64'),
    ],
)

function(name='glProgramUniform3i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLint64EXT'),
        Argument(name='y', type='GLint64EXT'),
        Argument(name='z', type='GLint64EXT'),
    ],
)

function(name='glProgramUniform3i64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64*', wrap_params='count * 3, value'),
    ],
)

function(name='glProgramUniform3i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64EXT*', wrap_params='count * 3, value'),
    ],
)

function(name='glProgramUniform3iEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform3i')

function(name='glProgramUniform3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint*', wrap_params='count * 3, value'),
    ],
)

function(name='glProgramUniform3ivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform3iv')

function(name='glProgramUniform3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='v0', type='GLuint'),
        Argument(name='v1', type='GLuint'),
        Argument(name='v2', type='GLuint'),
    ],
)

function(name='glProgramUniform3ui64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLuint64'),
        Argument(name='y', type='GLuint64'),
        Argument(name='z', type='GLuint64'),
    ],
)

function(name='glProgramUniform3ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='x', type='GLuint64EXT'),
        Argument(name='y', type='GLuint64EXT'),
        Argument(name='z', type='GLuint64EXT'),
    ],
)

function(name='glProgramUniform3ui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_params='count * 3, value'),
    ],
)

function(name='glProgramUniform3ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count * 3, value'),
    ],
)

function(name='glProgramUniform3uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform3ui')

function(name='glProgramUniform3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint*', wrap_params='count * 3, value'),
    ],
)

function(name='glProgramUniform3uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform3uiv')

function(name='glProgramUniform4d', enabled=True, function_type=FuncType.PARAM,
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

function(name='glProgramUniform4dEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform4d')

function(name='glProgramUniform4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 4, value'),
    ],
)

function(name='glProgramUniform4dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform4dv')

function(name='glProgramUniform4f', enabled=True, function_type=FuncType.PARAM,
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

function(name='glProgramUniform4fEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform4f')

function(name='glProgramUniform4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 4, value'),
    ],
)

function(name='glProgramUniform4fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform4fv')

function(name='glProgramUniform4i', enabled=True, function_type=FuncType.PARAM,
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

function(name='glProgramUniform4i64ARB', enabled=False, function_type=FuncType.NONE,
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

function(name='glProgramUniform4i64NV', enabled=True, function_type=FuncType.PARAM,
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

function(name='glProgramUniform4i64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64*', wrap_params='count * 4, value'),
    ],
)

function(name='glProgramUniform4i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64EXT*', wrap_params='count * 4, value'),
    ],
)

function(name='glProgramUniform4iEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform4i')

function(name='glProgramUniform4iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint*', wrap_params='count * 4, value'),
    ],
)

function(name='glProgramUniform4ivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform4iv')

function(name='glProgramUniform4ui', enabled=True, function_type=FuncType.PARAM,
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

function(name='glProgramUniform4ui64ARB', enabled=False, function_type=FuncType.NONE,
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

function(name='glProgramUniform4ui64NV', enabled=True, function_type=FuncType.PARAM,
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

function(name='glProgramUniform4ui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_params='count * 4, value'),
    ],
)

function(name='glProgramUniform4ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count * 4, value'),
    ],
)

function(name='glProgramUniform4uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform4ui')

function(name='glProgramUniform4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint*', wrap_params='count * 4, value'),
    ],
)

function(name='glProgramUniform4uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniform4uiv')

function(name='glProgramUniformHandleui64ARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='value', type='GLuint64', wrap_type='CGLTextureHandle'),
    ],
)

function(name='glProgramUniformHandleui64IMG', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='value', type='GLuint64'),
    ],
)

function(name='glProgramUniformHandleui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='value', type='GLuint64'),
    ],
)

function(name='glProgramUniformHandleui64vARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='values', type='const GLuint64*', wrap_type='CGLTextureHandle::CSArray', wrap_params='count, values'),
    ],
)

function(name='glProgramUniformHandleui64vIMG', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='values', type='const GLuint64*'),
    ],
)

function(name='glProgramUniformHandleui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='values', type='const GLuint64*', wrap_params='count, values'),
    ],
)

function(name='glProgramUniformMatrix2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 4, value'),
    ],
)

function(name='glProgramUniformMatrix2dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix2dv')

function(name='glProgramUniformMatrix2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 4, value'),
    ],
)

function(name='glProgramUniformMatrix2fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix2fv')

function(name='glProgramUniformMatrix2x3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 6, value'),
    ],
)

function(name='glProgramUniformMatrix2x3dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix2x3dv')

function(name='glProgramUniformMatrix2x3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 6, value'),
    ],
)

function(name='glProgramUniformMatrix2x3fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix2x3fv')

function(name='glProgramUniformMatrix2x4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 8, value'),
    ],
)

function(name='glProgramUniformMatrix2x4dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix2x4dv')

function(name='glProgramUniformMatrix2x4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 8, value'),
    ],
)

function(name='glProgramUniformMatrix2x4fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix2x4fv')

function(name='glProgramUniformMatrix3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 9, value'),
    ],
)

function(name='glProgramUniformMatrix3dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix3dv')

function(name='glProgramUniformMatrix3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 9, value'),
    ],
)

function(name='glProgramUniformMatrix3fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix3fv')

function(name='glProgramUniformMatrix3x2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 6, value'),
    ],
)

function(name='glProgramUniformMatrix3x2dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix3x2dv')

function(name='glProgramUniformMatrix3x2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 6, value'),
    ],
)

function(name='glProgramUniformMatrix3x2fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix3x2fv')

function(name='glProgramUniformMatrix3x4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 12, value'),
    ],
)

function(name='glProgramUniformMatrix3x4dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix3x4dv')

function(name='glProgramUniformMatrix3x4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 12, value'),
    ],
)

function(name='glProgramUniformMatrix3x4fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix3x4fv')

function(name='glProgramUniformMatrix4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 16, value'),
    ],
)

function(name='glProgramUniformMatrix4dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix4dv')

function(name='glProgramUniformMatrix4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 16, value'),
    ],
)

function(name='glProgramUniformMatrix4fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix4fv')

function(name='glProgramUniformMatrix4x2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 8, value'),
    ],
)

function(name='glProgramUniformMatrix4x2dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix4x2dv')

function(name='glProgramUniformMatrix4x2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 8, value'),
    ],
)

function(name='glProgramUniformMatrix4x2fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix4x2fv')

function(name='glProgramUniformMatrix4x3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 12, value'),
    ],
)

function(name='glProgramUniformMatrix4x3dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix4x3dv')

function(name='glProgramUniformMatrix4x3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 12, value'),
    ],
)

function(name='glProgramUniformMatrix4x3fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProgramUniformMatrix4x3fv')

function(name='glProgramUniformui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='value', type='GLuint64EXT'),
    ],
)

function(name='glProgramUniformui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='program, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count, value'),
    ],
)

function(name='glProgramVertexLimitNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='limit', type='GLint'),
    ],
)

function(name='glProvokingVertex', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glProvokingVertexEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glProvokingVertex')

function(name='glPushAttrib', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLbitfield'),
    ],
)

function(name='glPushClientAttrib', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLbitfield'),
    ],
)

function(name='glPushClientAttribDefaultEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLbitfield'),
    ],
)

function(name='glPushDebugGroup', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='source', type='GLenum'),
        Argument(name='id', type='GLuint'),
        Argument(name='length', type='GLsizei'),
        Argument(name='message', type='const GLchar*', wrap_params='message, \'\\0\', 1'),
    ],
)

function(name='glPushDebugGroupKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glPushDebugGroup')

function(name='glPushGroupMarkerEXT', enabled=True, function_type=FuncType.PARAM, interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='length', type='GLsizei'),
        Argument(name='marker', type='const GLchar*', wrap_params='length, marker'),
    ],
)

function(name='glPushMatrix', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glPushName', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='name', type='GLuint'),
    ],
)

function(name='glQueryCounter', enabled=True, function_type=FuncType.QUERY, run_condition='ConditionQueries()',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint', wrap_type='CGLQuery'),
        Argument(name='target', type='GLenum'),
    ],
)

function(name='glQueryCounterEXT', enabled=True, function_type=FuncType.QUERY, inherit_from='glQueryCounter')

function(name='glQueryCounterINTEL', enabled=True, function_type=FuncType.QUERY, inherit_from='glQueryCounter')

function(name='glQueryMatrixxOES', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='GLbitfield'),
    args=[
        Argument(name='mantissa', type='GLfixed*', wrap_params='16, mantissa'),
        Argument(name='exponent', type='GLint*', wrap_params='16, exponent'),
    ],
)

function(name='glQueryObjectParameteruiAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='id', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLuint'),
    ],
)

function(name='glQueryResourceNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='queryType', type='GLenum'),
        Argument(name='tagId', type='GLint'),
        Argument(name='bufSize', type='GLuint'),
        Argument(name='buffer', type='GLint*'),
    ],
)

function(name='glQueryResourceTagNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tagId', type='GLint'),
        Argument(name='tagString', type='const GLchar*'),
    ],
)

function(name='glRasterPos2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
    ],
)

function(name='glRasterPos2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='2, v'),
    ],
)

function(name='glRasterPos2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
    ],
)

function(name='glRasterPos2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='2, v'),
    ],
)

function(name='glRasterPos2i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
    ],
)

function(name='glRasterPos2iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='2, v'),
    ],
)

function(name='glRasterPos2s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
    ],
)

function(name='glRasterPos2sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='2, v'),
    ],
)

function(name='glRasterPos2xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfixed'),
        Argument(name='y', type='GLfixed'),
    ],
)

function(name='glRasterPos2xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

function(name='glRasterPos3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

function(name='glRasterPos3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

function(name='glRasterPos3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

function(name='glRasterPos3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

function(name='glRasterPos3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
    ],
)

function(name='glRasterPos3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

function(name='glRasterPos3s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
    ],
)

function(name='glRasterPos3sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

function(name='glRasterPos3xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfixed'),
        Argument(name='y', type='GLfixed'),
        Argument(name='z', type='GLfixed'),
    ],
)

function(name='glRasterPos3xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

function(name='glRasterPos4d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

function(name='glRasterPos4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='4, v'),
    ],
)

function(name='glRasterPos4f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
    ],
)

function(name='glRasterPos4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

function(name='glRasterPos4i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
        Argument(name='w', type='GLint'),
    ],
)

function(name='glRasterPos4iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

function(name='glRasterPos4s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
        Argument(name='w', type='GLshort'),
    ],
)

function(name='glRasterPos4sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='4, v'),
    ],
)

function(name='glRasterPos4xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfixed'),
        Argument(name='y', type='GLfixed'),
        Argument(name='z', type='GLfixed'),
        Argument(name='w', type='GLfixed'),
    ],
)

function(name='glRasterPos4xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

function(name='glRasterSamplesEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='samples', type='GLuint'),
        Argument(name='fixedsamplelocations', type='GLboolean'),
    ],
)

function(name='glReadBuffer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='src', type='GLenum'),
    ],
)

function(name='glReadBufferIndexedEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='src', type='GLenum'),
        Argument(name='index', type='GLint'),
    ],
)

function(name='glReadBufferNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glReadBuffer')

function(name='glReadInstrumentsSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='marker', type='GLint'),
    ],
)

function(name='glReadPixels', enabled=True, function_type=FuncType.PARAM, run_wrap=True,
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

function(name='glReadnPixels', enabled=False, function_type=FuncType.NONE,
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

function(name='glReadnPixelsARB', enabled=False, function_type=FuncType.NONE, inherit_from='glReadnPixels')

function(name='glReadnPixelsEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glReadnPixels')

function(name='glReadnPixelsKHR', enabled=False, function_type=FuncType.NONE, inherit_from='glReadnPixels')

function(name='glRectd', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x1', type='GLdouble'),
        Argument(name='y1', type='GLdouble'),
        Argument(name='x2', type='GLdouble'),
        Argument(name='y2', type='GLdouble'),
    ],
)

function(name='glRectdv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v1', type='const GLdouble*', wrap_params='2, v1'),
        Argument(name='v2', type='const GLdouble*', wrap_params='2, v2'),
    ],
)

function(name='glRectf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x1', type='GLfloat'),
        Argument(name='y1', type='GLfloat'),
        Argument(name='x2', type='GLfloat'),
        Argument(name='y2', type='GLfloat'),
    ],
)

function(name='glRectfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v1', type='const GLfloat*', wrap_params='2, v1'),
        Argument(name='v2', type='const GLfloat*', wrap_params='2, v2'),
    ],
)

function(name='glRecti', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x1', type='GLint'),
        Argument(name='y1', type='GLint'),
        Argument(name='x2', type='GLint'),
        Argument(name='y2', type='GLint'),
    ],
)

function(name='glRectiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v1', type='const GLint*', wrap_params='2, v1'),
        Argument(name='v2', type='const GLint*', wrap_params='2, v2'),
    ],
)

function(name='glRects', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x1', type='GLshort'),
        Argument(name='y1', type='GLshort'),
        Argument(name='x2', type='GLshort'),
        Argument(name='y2', type='GLshort'),
    ],
)

function(name='glRectsv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v1', type='const GLshort*', wrap_params='2, v1'),
        Argument(name='v2', type='const GLshort*', wrap_params='2, v2'),
    ],
)

function(name='glRectxOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x1', type='GLfixed'),
        Argument(name='y1', type='GLfixed'),
        Argument(name='x2', type='GLfixed'),
        Argument(name='y2', type='GLfixed'),
    ],
)

function(name='glRectxvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v1', type='const GLfixed*'),
        Argument(name='v2', type='const GLfixed*'),
    ],
)

function(name='glReferencePlaneSGIX', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='equation', type='const GLdouble*'),
    ],
)

function(name='glReleaseKeyedMutexWin32EXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='memory', type='GLuint'),
        Argument(name='key', type='GLuint64'),
    ],
)

function(name='glReleaseShaderCompiler', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glRenderGpuMaskNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLbitfield'),
    ],
)

function(name='glRenderMode', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='GLint'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glRenderbufferStorage', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

function(name='glRenderbufferStorageEXT', enabled=True, function_type=FuncType.PARAM, recorder_wrap=True, inherit_from='glRenderbufferStorage')

function(name='glRenderbufferStorageMultisample', enabled=True, function_type=FuncType.PARAM, run_wrap=True, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='samples', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

function(name='glRenderbufferStorageMultisampleANGLE', enabled=True, function_type=FuncType.PARAM, run_wrap=True, inherit_from='glRenderbufferStorageMultisample')

function(name='glRenderbufferStorageMultisampleAPPLE', enabled=False, function_type=FuncType.PARAM, inherit_from='glRenderbufferStorageMultisample')

function(name='glRenderbufferStorageMultisampleCoverageNV', enabled=True, function_type=FuncType.PARAM, run_wrap=True,
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

function(name='glRenderbufferStorageMultisampleEXT', enabled=True, function_type=FuncType.PARAM, run_wrap=True, state_track=True, recorder_wrap=True, inherit_from='glRenderbufferStorageMultisample')

function(name='glRenderbufferStorageMultisampleIMG', enabled=True, function_type=FuncType.PARAM, run_wrap=True, inherit_from='glRenderbufferStorageMultisample')

function(name='glRenderbufferStorageMultisampleNV', enabled=False, function_type=FuncType.NONE, inherit_from='glRenderbufferStorageMultisample')

function(name='glRenderbufferStorageOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glRenderbufferStorage')

function(name='glReplacementCodePointerSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void**'),
    ],
)

function(name='glReplacementCodeubSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='code', type='GLubyte'),
    ],
)

function(name='glReplacementCodeubvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='code', type='const GLubyte*'),
    ],
)

function(name='glReplacementCodeuiColor3fVertex3fSUN', enabled=False, function_type=FuncType.NONE,
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

function(name='glReplacementCodeuiColor3fVertex3fvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='const GLuint*'),
        Argument(name='c', type='const GLfloat*'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glReplacementCodeuiColor4fNormal3fVertex3fSUN', enabled=False, function_type=FuncType.NONE,
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

function(name='glReplacementCodeuiColor4fNormal3fVertex3fvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='const GLuint*'),
        Argument(name='c', type='const GLfloat*'),
        Argument(name='n', type='const GLfloat*'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glReplacementCodeuiColor4ubVertex3fSUN', enabled=False, function_type=FuncType.NONE,
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

function(name='glReplacementCodeuiColor4ubVertex3fvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='const GLuint*'),
        Argument(name='c', type='const GLubyte*'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glReplacementCodeuiNormal3fVertex3fSUN', enabled=False, function_type=FuncType.NONE,
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

function(name='glReplacementCodeuiNormal3fVertex3fvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='const GLuint*'),
        Argument(name='n', type='const GLfloat*'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glReplacementCodeuiSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='code', type='GLuint'),
    ],
)

function(name='glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN', enabled=False, function_type=FuncType.NONE,
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

function(name='glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='const GLuint*'),
        Argument(name='tc', type='const GLfloat*'),
        Argument(name='c', type='const GLfloat*'),
        Argument(name='n', type='const GLfloat*'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN', enabled=False, function_type=FuncType.NONE,
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

function(name='glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='const GLuint*'),
        Argument(name='tc', type='const GLfloat*'),
        Argument(name='n', type='const GLfloat*'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glReplacementCodeuiTexCoord2fVertex3fSUN', enabled=False, function_type=FuncType.NONE,
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

function(name='glReplacementCodeuiTexCoord2fVertex3fvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='const GLuint*'),
        Argument(name='tc', type='const GLfloat*'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glReplacementCodeuiVertex3fSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='GLuint'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

function(name='glReplacementCodeuiVertex3fvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='rc', type='const GLuint*'),
        Argument(name='v', type='const GLfloat*'),
    ],
)

function(name='glReplacementCodeuivSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='code', type='const GLuint*'),
    ],
)

function(name='glReplacementCodeusSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='code', type='GLushort'),
    ],
)

function(name='glReplacementCodeusvSUN', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='code', type='const GLushort*'),
    ],
)

function(name='glRequestResidentProgramsNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='n', type='GLsizei'),
        Argument(name='programs', type='const GLuint*', wrap_params='n, programs'),
    ],
)

function(name='glResetHistogram', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

function(name='glResetHistogramEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glResetHistogram')

function(name='glResetMinmax', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

function(name='glResetMinmaxEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glResetMinmax')

function(name='glResizeBuffersMESA', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glResolveDepthValuesNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glResolveMultisampleFramebufferAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glResumeTransformFeedback', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glResumeTransformFeedbackNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glResumeTransformFeedback')

function(name='glRotated', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='angle', type='GLdouble'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

function(name='glRotatef', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='angle', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

function(name='glRotatex', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='angle', type='GLfixed'),
        Argument(name='x', type='GLfixed'),
        Argument(name='y', type='GLfixed'),
        Argument(name='z', type='GLfixed'),
    ],
)

function(name='glRotatexOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glRotatex')

function(name='glSampleCoverage', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='value', type='GLfloat'),
        Argument(name='invert', type='GLboolean'),
    ],
)

function(name='glSampleCoverageARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glSampleCoverage')

function(name='glSampleCoverageOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='value', type='GLfixed'),
        Argument(name='invert', type='GLboolean'),
    ],
)

function(name='glSampleCoveragex', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='value', type='GLclampx'),
        Argument(name='invert', type='GLboolean'),
    ],
)

function(name='glSampleCoveragexOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glSampleCoveragex')

function(name='glSampleMapATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='dst', type='GLuint'),
        Argument(name='interp', type='GLuint'),
        Argument(name='swizzle', type='GLenum'),
    ],
)

function(name='glSampleMaskEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='value', type='GLclampf'),
        Argument(name='invert', type='GLboolean'),
    ],
)

function(name='glSampleMaskIndexedNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='mask', type='GLbitfield'),
    ],
)

function(name='glSampleMaskSGIS', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='value', type='GLclampf'),
        Argument(name='invert', type='GLboolean'),
    ],
)

function(name='glSampleMaski', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='maskNumber', type='GLuint'),
        Argument(name='mask', type='GLbitfield'),
    ],
)

function(name='glSamplePass', enabled=False, function_type=FuncType.PARAM, interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glSamplePatternEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pattern', type='GLenum'),
    ],
)

function(name='glSamplePatternSGIS', enabled=True, function_type=FuncType.PARAM, inherit_from='glSamplePatternEXT')

function(name='glSamplerParameterIiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, param'),
    ],
)

function(name='glSamplerParameterIivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glSamplerParameterIiv')

function(name='glSamplerParameterIivOES', enabled=False, function_type=FuncType.NONE, inherit_from='glSamplerParameterIiv')

function(name='glSamplerParameterIuiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='const GLuint*', wrap_type='CGLuint::CSParamArray', wrap_params='pname, param'),
    ],
)

function(name='glSamplerParameterIuivEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glSamplerParameterIuiv')

function(name='glSamplerParameterIuivOES', enabled=False, function_type=FuncType.NONE, inherit_from='glSamplerParameterIuiv')

function(name='glSamplerParameterf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glSamplerParameterfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, param'),
    ],
)

function(name='glSamplerParameteri', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glSamplerParameteriv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sampler', type='GLuint', wrap_type='CGLSampler'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, param'),
    ],
)

function(name='glScaled', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

function(name='glScalef', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

function(name='glScalex', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfixed'),
        Argument(name='y', type='GLfixed'),
        Argument(name='z', type='GLfixed'),
    ],
)

function(name='glScalexOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glScalex')

function(name='glScissor', enabled=True, function_type=FuncType.PARAM, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

function(name='glScissorArrayv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLint*', wrap_params='4 * count, v'),
    ],
)

function(name='glScissorArrayvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glScissorArrayv')

function(name='glScissorArrayvOES', enabled=False, function_type=FuncType.NONE, inherit_from='glScissorArrayv')

function(name='glScissorIndexed', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='left', type='GLint'),
        Argument(name='bottom', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

function(name='glScissorIndexedNV', enabled=False, function_type=FuncType.NONE, inherit_from='glScissorIndexed')

function(name='glScissorIndexedOES', enabled=False, function_type=FuncType.NONE, inherit_from='glScissorIndexed')

function(name='glScissorIndexedv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

function(name='glScissorIndexedvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glScissorIndexedv')

function(name='glScissorIndexedvOES', enabled=False, function_type=FuncType.NONE, inherit_from='glScissorIndexedv')

function(name='glSecondaryColor3b', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLbyte'),
        Argument(name='green', type='GLbyte'),
        Argument(name='blue', type='GLbyte'),
    ],
)

function(name='glSecondaryColor3bEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3b')

function(name='glSecondaryColor3bv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLbyte*', wrap_params='3, v'),
    ],
)

function(name='glSecondaryColor3bvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3bv')

function(name='glSecondaryColor3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLdouble'),
        Argument(name='green', type='GLdouble'),
        Argument(name='blue', type='GLdouble'),
    ],
)

function(name='glSecondaryColor3dEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3d')

function(name='glSecondaryColor3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

function(name='glSecondaryColor3dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3dv')

function(name='glSecondaryColor3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLfloat'),
        Argument(name='green', type='GLfloat'),
        Argument(name='blue', type='GLfloat'),
    ],
)

function(name='glSecondaryColor3fEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3f')

function(name='glSecondaryColor3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

function(name='glSecondaryColor3fvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3fv')

function(name='glSecondaryColor3hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLhalfNV'),
        Argument(name='green', type='GLhalfNV'),
        Argument(name='blue', type='GLhalfNV'),
    ],
)

function(name='glSecondaryColor3hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='3, v'),
    ],
)

function(name='glSecondaryColor3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLint'),
        Argument(name='green', type='GLint'),
        Argument(name='blue', type='GLint'),
    ],
)

function(name='glSecondaryColor3iEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3i')

function(name='glSecondaryColor3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

function(name='glSecondaryColor3ivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3iv')

function(name='glSecondaryColor3s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLshort'),
        Argument(name='green', type='GLshort'),
        Argument(name='blue', type='GLshort'),
    ],
)

function(name='glSecondaryColor3sEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3s')

function(name='glSecondaryColor3sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

function(name='glSecondaryColor3svEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3sv')

function(name='glSecondaryColor3ub', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLubyte'),
        Argument(name='green', type='GLubyte'),
        Argument(name='blue', type='GLubyte'),
    ],
)

function(name='glSecondaryColor3ubEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3ub')

function(name='glSecondaryColor3ubv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLubyte*', wrap_params='3, v'),
    ],
)

function(name='glSecondaryColor3ubvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3ubv')

function(name='glSecondaryColor3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLuint'),
        Argument(name='green', type='GLuint'),
        Argument(name='blue', type='GLuint'),
    ],
)

function(name='glSecondaryColor3uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3ui')

function(name='glSecondaryColor3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLuint*', wrap_params='3, v'),
    ],
)

function(name='glSecondaryColor3uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3uiv')

function(name='glSecondaryColor3us', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLushort'),
        Argument(name='green', type='GLushort'),
        Argument(name='blue', type='GLushort'),
    ],
)

function(name='glSecondaryColor3usEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3us')

function(name='glSecondaryColor3usv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLushort*', wrap_params='3, v'),
    ],
)

function(name='glSecondaryColor3usvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColor3usv')

function(name='glSecondaryColorFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
    ],
)

function(name='glSecondaryColorP3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='color', type='GLuint'),
    ],
)

function(name='glSecondaryColorP3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='color', type='const GLuint*', wrap_params='1, color'),
    ],
)

function(name='glSecondaryColorPointer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

function(name='glSecondaryColorPointerEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glSecondaryColorPointer')

function(name='glSecondaryColorPointerListIBM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLint'),
        Argument(name='pointer', type='const void**'),
        Argument(name='ptrstride', type='GLint'),
    ],
)

function(name='glSelectBuffer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLsizei'),
        Argument(name='buffer', type='GLuint*', wrap_params='size, buffer'),
    ],
)

function(name='glSelectPerfMonitorCountersAMD', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='monitor', type='GLuint'),
        Argument(name='enable', type='GLboolean'),
        Argument(name='group', type='GLuint'),
        Argument(name='numCounters', type='GLint'),
        Argument(name='counterList', type='GLuint*', wrap_params='numCounters, counterList'),
    ],
)

function(name='glSemaphoreParameterivNV', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='semaphore', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glSemaphoreParameterui64vEXT', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='semaphore', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLuint64*'),
    ],
)

function(name='glSeparableFilter2D', enabled=False, function_type=FuncType.NONE,
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

function(name='glSeparableFilter2DEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glSeparableFilter2D')

function(name='glSetFenceAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='fence', type='GLuint'),
    ],
)

function(name='glSetFenceNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='fence', type='GLuint'),
        Argument(name='condition', type='GLenum'),
    ],
)

function(name='glSetFragmentShaderConstantATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='dst', type='GLuint'),
        Argument(name='value', type='const GLfloat*', wrap_params='4, value'),
    ],
)

function(name='glSetInvariantEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='addr', type='const void*'),
    ],
)

function(name='glSetLocalConstantEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='addr', type='const void*'),
    ],
)

function(name='glSetMultisamplefvAMD', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='index', type='GLuint'),
        Argument(name='val', type='const GLfloat*', wrap_params='1, val'),
    ],
)

function(name='glShadeModel', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glShaderBinary', enabled=True, function_type=FuncType.PARAM, interceptor_exec_override=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='count', type='GLsizei'),
        Argument(name='shaders', type='const GLuint*', wrap_params='count, shaders'),
        Argument(name='binaryformat', type='GLenum'),
        Argument(name='binary', type='const void*', wrap_type='CBinaryResource', wrap_params='RESOURCE_BUFFER, binary, length'),
        Argument(name='length', type='GLsizei'),
    ],
)

function(name='glShaderOp1EXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='op', type='GLenum'),
        Argument(name='res', type='GLuint'),
        Argument(name='arg1', type='GLuint'),
    ],
)

function(name='glShaderOp2EXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='op', type='GLenum'),
        Argument(name='res', type='GLuint'),
        Argument(name='arg1', type='GLuint'),
        Argument(name='arg2', type='GLuint'),
    ],
)

function(name='glShaderOp3EXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='op', type='GLenum'),
        Argument(name='res', type='GLuint'),
        Argument(name='arg1', type='GLuint'),
        Argument(name='arg2', type='GLuint'),
        Argument(name='arg3', type='GLuint'),
    ],
)

function(name='glShaderSource', enabled=True, function_type=FuncType.RESOURCE, state_track=True, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='count', type='GLsizei', wrap_params='1'),
        Argument(name='string', type='const GLchar*const*', wrap_type='CShaderSource', wrap_params='shader, count, string, length, _string.SHADER_SOURCE'),
        Argument(name='length', type='const GLint*', wrap_type='CGLintptrZero', wrap_params=' '),
    ],
)

function(name='glShaderSourceARB', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glShaderSource', run_wrap=True,
    args=[
        Argument(name='shaderObj', type='GLhandleARB', wrap_type='CGLProgram'),
        Argument(name='count', type='GLsizei', wrap_params='1'),
        # TODO: GL_ARB_shader_objects and gl.xml specify it as `const GLcharARB**` (without the second const).
        Argument(name='string', type='const GLcharARB*const*', wrap_type='CShaderSource', wrap_params='shaderObj, count, string, length, _string.SHADER_SOURCE'),
        Argument(name='length', type='const GLint*', wrap_type='CGLintptrZero', wrap_params=' '),
    ],
)

function(name='glShaderStorageBlockBinding', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='storageBlockIndex', type='GLuint'),
        Argument(name='storageBlockBinding', type='GLuint'),
    ],
)

function(name='glShaderStorageBlockBinding', enabled=True, function_type=FuncType.PARAM, version=1,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='storageBlockIndex', type='GLuint',wrap_type='CGLStorageBlockIndex', wrap_params='program, storageBlockIndex'),
        Argument(name='storageBlockBinding', type='GLuint'),
    ],
)

function(name='glSharpenTexFuncSGIS', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='n', type='GLsizei'),
        Argument(name='points', type='const GLfloat*', wrap_params='n * 2, points'),
    ],
)

function(name='glSignalSemaphoreEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glSignalVkFenceNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vkFence', type='GLuint64'),
    ],
)

function(name='glSignalVkSemaphoreNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vkSemaphore', type='GLuint64'),
    ],
)

function(name='glSpecializeShader', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shader', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='pEntryPoint', type='const GLchar*'),
        Argument(name='numSpecializationConstants', type='GLuint'),
        Argument(name='pConstantIndex', type='const GLuint*'),
        Argument(name='pConstantValue', type='const GLuint*'),
    ],
)

function(name='glSpecializeShaderARB', enabled=False, function_type=FuncType.NONE, inherit_from='glSpecializeShader')

function(name='glSpriteParameterfSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glSpriteParameterfvSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glSpriteParameteriSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glSpriteParameterivSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glStartInstrumentsSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glStartTilingQCOM', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLuint'),
        Argument(name='y', type='GLuint'),
        Argument(name='width', type='GLuint'),
        Argument(name='height', type='GLuint'),
        Argument(name='preserveMask', type='GLbitfield'),
    ],
)

function(name='glStateCaptureNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='state', type='GLuint'),
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glStencilClearTagEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stencilTagBits', type='GLsizei'),
        Argument(name='stencilClearTag', type='GLuint'),
    ],
)

function(name='glStencilFillPathInstancedNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glStencilFillPathNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='fillMode', type='GLenum'),
        Argument(name='mask', type='GLuint'),
    ],
)

function(name='glStencilFunc', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='func', type='GLenum'),
        Argument(name='ref', type='GLint'),
        Argument(name='mask', type='GLuint'),
    ],
)

function(name='glStencilFuncSeparate', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='func', type='GLenum'),
        Argument(name='ref', type='GLint'),
        Argument(name='mask', type='GLuint'),
    ],
)

function(name='glStencilFuncSeparateATI', enabled=True, function_type=FuncType.PARAM, inherit_from='glStencilFuncSeparate')

function(name='glStencilMask', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLuint'),
    ],
)

function(name='glStencilMaskSeparate', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='mask', type='GLuint'),
    ],
)

function(name='glStencilOp', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='fail', type='GLenum'),
        Argument(name='zfail', type='GLenum'),
        Argument(name='zpass', type='GLenum'),
    ],
)

function(name='glStencilOpSeparate', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='sfail', type='GLenum'),
        Argument(name='dpfail', type='GLenum'),
        Argument(name='dppass', type='GLenum'),
    ],
)

function(name='glStencilOpSeparateATI', enabled=True, function_type=FuncType.PARAM, inherit_from='glStencilOpSeparate')

function(name='glStencilOpValueAMD', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='value', type='GLuint'),
    ],
)

function(name='glStencilStrokePathInstancedNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glStencilStrokePathNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='reference', type='GLint'),
        Argument(name='mask', type='GLuint'),
    ],
)

function(name='glStencilThenCoverFillPathInstancedNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glStencilThenCoverFillPathNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='fillMode', type='GLenum'),
        Argument(name='mask', type='GLuint'),
        Argument(name='coverMode', type='GLenum'),
    ],
)

function(name='glStencilThenCoverStrokePathInstancedNV', enabled=False, function_type=FuncType.NONE,
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

function(name='glStencilThenCoverStrokePathNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='path', type='GLuint'),
        Argument(name='reference', type='GLint'),
        Argument(name='mask', type='GLuint'),
        Argument(name='coverMode', type='GLenum'),
    ],
)

function(name='glStopInstrumentsSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='marker', type='GLint'),
    ],
)

function(name='glStringMarkerGREMEDY', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='len', type='GLsizei'),
        Argument(name='string', type='const void*'),
    ],
)

function(name='glSubpixelPrecisionBiasNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='xbits', type='GLuint'),
        Argument(name='ybits', type='GLuint'),
    ],
)

function(name='glSwapAPPLE', enabled=False, function_type=FuncType.EXEC,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glSwizzleEXT', enabled=True, function_type=FuncType.PARAM,
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

function(name='glSyncTextureINTEL', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
    ],
)

function(name='glTagSampleBufferSGIX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glTangent3bEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tx', type='GLbyte'),
        Argument(name='ty', type='GLbyte'),
        Argument(name='tz', type='GLbyte'),
    ],
)

function(name='glTangent3bvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLbyte*', wrap_params='3, v'),
    ],
)

function(name='glTangent3dEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tx', type='GLdouble'),
        Argument(name='ty', type='GLdouble'),
        Argument(name='tz', type='GLdouble'),
    ],
)

function(name='glTangent3dvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

function(name='glTangent3fEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tx', type='GLfloat'),
        Argument(name='ty', type='GLfloat'),
        Argument(name='tz', type='GLfloat'),
    ],
)

function(name='glTangent3fvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

function(name='glTangent3iEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tx', type='GLint'),
        Argument(name='ty', type='GLint'),
        Argument(name='tz', type='GLint'),
    ],
)

function(name='glTangent3ivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

function(name='glTangent3sEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tx', type='GLshort'),
        Argument(name='ty', type='GLshort'),
        Argument(name='tz', type='GLshort'),
    ],
)

function(name='glTangent3svEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

function(name='glTangentPointerEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glTbufferMask3DFX', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mask', type='GLuint'),
    ],
)

function(name='glTessellationFactorAMD', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='factor', type='GLfloat'),
    ],
)

function(name='glTessellationModeAMD', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glTestFenceAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='fence', type='GLuint'),
    ],
)

function(name='glTestFenceNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='fence', type='GLuint'),
    ],
)

function(name='glTestObjectAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='object', type='GLenum'),
        Argument(name='name', type='GLuint'),
    ],
)

function(name='glTexBuffer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

function(name='glTexBufferARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexBuffer')

function(name='glTexBufferEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexBuffer')

function(name='glTexBufferOES', enabled=False, function_type=FuncType.NONE, inherit_from='glTexBuffer')

function(name='glTexBufferRange', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
    ],
)

function(name='glTexBufferRangeEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glTexBufferRange')

function(name='glTexBufferRangeOES', enabled=False, function_type=FuncType.NONE, inherit_from='glTexBufferRange')

function(name='glTexBumpParameterfvATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, param'),
    ],
)

function(name='glTexBumpParameterivATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, param'),
    ],
)

function(name='glTexCoord1bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLbyte'),
    ],
)

function(name='glTexCoord1bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLbyte*'),
    ],
)

function(name='glTexCoord1d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLdouble'),
    ],
)

function(name='glTexCoord1dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='1, v'),
    ],
)

function(name='glTexCoord1f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfloat'),
    ],
)

function(name='glTexCoord1fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='1, v'),
    ],
)

function(name='glTexCoord1hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLhalfNV'),
    ],
)

function(name='glTexCoord1hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='1, v'),
    ],
)

function(name='glTexCoord1i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLint'),
    ],
)

function(name='glTexCoord1iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='1, v'),
    ],
)

function(name='glTexCoord1s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLshort'),
    ],
)

function(name='glTexCoord1sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='1, v'),
    ],
)

function(name='glTexCoord1xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfixed'),
    ],
)

function(name='glTexCoord1xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

function(name='glTexCoord2bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLbyte'),
        Argument(name='t', type='GLbyte'),
    ],
)

function(name='glTexCoord2bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLbyte*'),
    ],
)

function(name='glTexCoord2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLdouble'),
        Argument(name='t', type='GLdouble'),
    ],
)

function(name='glTexCoord2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='2, v'),
    ],
)

function(name='glTexCoord2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
    ],
)

function(name='glTexCoord2fColor3fVertex3fSUN', enabled=True, function_type=FuncType.PARAM,
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

function(name='glTexCoord2fColor3fVertex3fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tc', type='const GLfloat*', wrap_params='3, tc'),
        Argument(name='c', type='const GLfloat*', wrap_params='3, c'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

function(name='glTexCoord2fColor4fNormal3fVertex3fSUN', enabled=True, function_type=FuncType.PARAM,
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

function(name='glTexCoord2fColor4fNormal3fVertex3fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tc', type='const GLfloat*', wrap_params='3, tc'),
        Argument(name='c', type='const GLfloat*', wrap_params='3, c'),
        Argument(name='n', type='const GLfloat*', wrap_params='3, n'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

function(name='glTexCoord2fColor4ubVertex3fSUN', enabled=True, function_type=FuncType.PARAM,
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

function(name='glTexCoord2fColor4ubVertex3fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tc', type='const GLfloat*', wrap_params='3, tc'),
        Argument(name='c', type='const GLubyte*', wrap_params='3, c'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

function(name='glTexCoord2fNormal3fVertex3fSUN', enabled=True, function_type=FuncType.PARAM,
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

function(name='glTexCoord2fNormal3fVertex3fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tc', type='const GLfloat*', wrap_params='3, tc'),
        Argument(name='n', type='const GLfloat*', wrap_params='3, n'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

function(name='glTexCoord2fVertex3fSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

function(name='glTexCoord2fVertex3fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tc', type='const GLfloat*', wrap_params='3, tc'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

function(name='glTexCoord2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='2, v'),
    ],
)

function(name='glTexCoord2hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLhalfNV'),
        Argument(name='t', type='GLhalfNV'),
    ],
)

function(name='glTexCoord2hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='2, v'),
    ],
)

function(name='glTexCoord2i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLint'),
        Argument(name='t', type='GLint'),
    ],
)

function(name='glTexCoord2iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='2, v'),
    ],
)

function(name='glTexCoord2s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLshort'),
        Argument(name='t', type='GLshort'),
    ],
)

function(name='glTexCoord2sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='2, v'),
    ],
)

function(name='glTexCoord2xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfixed'),
        Argument(name='t', type='GLfixed'),
    ],
)

function(name='glTexCoord2xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

function(name='glTexCoord3bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLbyte'),
        Argument(name='t', type='GLbyte'),
        Argument(name='r', type='GLbyte'),
    ],
)

function(name='glTexCoord3bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLbyte*'),
    ],
)

function(name='glTexCoord3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLdouble'),
        Argument(name='t', type='GLdouble'),
        Argument(name='r', type='GLdouble'),
    ],
)

function(name='glTexCoord3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

function(name='glTexCoord3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='r', type='GLfloat'),
    ],
)

function(name='glTexCoord3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

function(name='glTexCoord3hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLhalfNV'),
        Argument(name='t', type='GLhalfNV'),
        Argument(name='r', type='GLhalfNV'),
    ],
)

function(name='glTexCoord3hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='3, v'),
    ],
)

function(name='glTexCoord3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLint'),
        Argument(name='t', type='GLint'),
        Argument(name='r', type='GLint'),
    ],
)

function(name='glTexCoord3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

function(name='glTexCoord3s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLshort'),
        Argument(name='t', type='GLshort'),
        Argument(name='r', type='GLshort'),
    ],
)

function(name='glTexCoord3sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

function(name='glTexCoord3xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfixed'),
        Argument(name='t', type='GLfixed'),
        Argument(name='r', type='GLfixed'),
    ],
)

function(name='glTexCoord3xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

function(name='glTexCoord4bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLbyte'),
        Argument(name='t', type='GLbyte'),
        Argument(name='r', type='GLbyte'),
        Argument(name='q', type='GLbyte'),
    ],
)

function(name='glTexCoord4bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLbyte*'),
    ],
)

function(name='glTexCoord4d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLdouble'),
        Argument(name='t', type='GLdouble'),
        Argument(name='r', type='GLdouble'),
        Argument(name='q', type='GLdouble'),
    ],
)

function(name='glTexCoord4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='4, v'),
    ],
)

function(name='glTexCoord4f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfloat'),
        Argument(name='t', type='GLfloat'),
        Argument(name='r', type='GLfloat'),
        Argument(name='q', type='GLfloat'),
    ],
)

function(name='glTexCoord4fColor4fNormal3fVertex4fSUN', enabled=True, function_type=FuncType.PARAM,
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

function(name='glTexCoord4fColor4fNormal3fVertex4fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tc', type='const GLfloat*', wrap_params='4, tc'),
        Argument(name='c', type='const GLfloat*', wrap_params='4, c'),
        Argument(name='n', type='const GLfloat*', wrap_params='4, n'),
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

function(name='glTexCoord4fVertex4fSUN', enabled=True, function_type=FuncType.PARAM,
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

function(name='glTexCoord4fVertex4fvSUN', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='tc', type='const GLfloat*', wrap_params='4, tc'),
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

function(name='glTexCoord4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

function(name='glTexCoord4hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLhalfNV'),
        Argument(name='t', type='GLhalfNV'),
        Argument(name='r', type='GLhalfNV'),
        Argument(name='q', type='GLhalfNV'),
    ],
)

function(name='glTexCoord4hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='4, v'),
    ],
)

function(name='glTexCoord4i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLint'),
        Argument(name='t', type='GLint'),
        Argument(name='r', type='GLint'),
        Argument(name='q', type='GLint'),
    ],
)

function(name='glTexCoord4iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

function(name='glTexCoord4s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLshort'),
        Argument(name='t', type='GLshort'),
        Argument(name='r', type='GLshort'),
        Argument(name='q', type='GLshort'),
    ],
)

function(name='glTexCoord4sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='4, v'),
    ],
)

function(name='glTexCoord4xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='s', type='GLfixed'),
        Argument(name='t', type='GLfixed'),
        Argument(name='r', type='GLfixed'),
        Argument(name='q', type='GLfixed'),
    ],
)

function(name='glTexCoord4xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

function(name='glTexCoordFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
    ],
)

function(name='glTexCoordP1ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='GLuint'),
    ],
)

function(name='glTexCoordP1uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='const GLuint*', wrap_params='1, coords'),
    ],
)

function(name='glTexCoordP2ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='GLuint'),
    ],
)

function(name='glTexCoordP2uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='const GLuint*', wrap_params='1, coords'),
    ],
)

function(name='glTexCoordP3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='GLuint'),
    ],
)

function(name='glTexCoordP3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='const GLuint*', wrap_params='1, coords'),
    ],
)

function(name='glTexCoordP4ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='GLuint'),
    ],
)

function(name='glTexCoordP4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='coords', type='const GLuint*', wrap_params='1, coords'),
    ],
)

function(name='glTexCoordPointer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

function(name='glTexCoordPointerBounds', enabled=True, function_type=FuncType.PARAM, run_wrap=True, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const GLvoid*', wrap_type='CAttribPtr'),
        Argument(name='count', type='GLsizei'),
    ],
)

function(name='glTexCoordPointerEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='count', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glTexCoordPointerListIBM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLint'),
        Argument(name='pointer', type='const void**'),
        Argument(name='ptrstride', type='GLint'),
    ],
)

function(name='glTexCoordPointervINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='pointer', type='const void**'),
    ],
)

function(name='glTexEnvf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glTexEnvfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glTexEnvi', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glTexEnviv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glTexEnvx', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

function(name='glTexEnvxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexEnvx')

function(name='glTexEnvxv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfixed*', wrap_type='CGLfixed::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glTexEnvxvOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexEnvxv')

function(name='glTexFilterFuncSGIS', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='filter', type='GLenum'),
        Argument(name='n', type='GLsizei'),
        Argument(name='weights', type='const GLfloat*', wrap_params='n, weights'),
    ],
)

function(name='glTexGend', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLdouble'),
    ],
)

function(name='glTexGendv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLdouble*', wrap_type='CGLdouble::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glTexGenf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glTexGenfOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexGenf')

function(name='glTexGenfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glTexGenfvOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexGenfv')

function(name='glTexGeni', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glTexGeniOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexGeni')

function(name='glTexGeniv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glTexGenivOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexGeniv')

function(name='glTexGenxOES', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

function(name='glTexGenxvOES', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coord', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfixed*', wrap_type='CGLfixed::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glTexImage1D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)', rec_condition='ConditionTexImageES(_recorder, format, type)',
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

function(name='glTexImage2D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionTexImageES(_recorder, format, type)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glTexImage2DMultisample', enabled=True, function_type=FuncType.PARAM, state_track=True, run_wrap=True,
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

function(name='glTexImage2DMultisampleCoverageNV', enabled=True, function_type=FuncType.PARAM,
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

function(name='glTexImage3D', enabled=True, function_type=FuncType.RESOURCE, state_track=True, rec_condition='ConditionTexImageES(_recorder, format, type)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glTexImage3DEXT', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glTexImage3D', rec_condition='ConditionTexImageES(_recorder, format, type)')

function(name='glTexImage3DMultisample', enabled=True, function_type=FuncType.PARAM, state_track=True,
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

function(name='glTexImage3DMultisampleCoverageNV', enabled=True, function_type=FuncType.PARAM,
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

function(name='glTexImage3DOES', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glTexImage3D')

function(name='glTexImage4DSGIS', enabled=False, function_type=FuncType.NONE,
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

function(name='glTexPageCommitmentARB', enabled=False, function_type=FuncType.NONE,
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

function(name='glTexPageCommitmentEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glTexParameterIiv', enabled=True, function_type=FuncType.PARAM, rec_condition='ConditionTexParamES(_recorder, pname)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glTexParameterIivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexParameterIiv')

function(name='glTexParameterIivOES', enabled=False, function_type=FuncType.NONE, inherit_from='glTexParameterIiv')

function(name='glTexParameterIuiv', enabled=True, function_type=FuncType.PARAM, rec_condition='ConditionTexParamES(_recorder, pname)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLuint*', wrap_type='CGLuint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glTexParameterIuivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexParameterIuiv')

function(name='glTexParameterIuivOES', enabled=False, function_type=FuncType.NONE, inherit_from='glTexParameterIuiv')

function(name='glTexParameterf', enabled=True, function_type=FuncType.PARAM, rec_condition='ConditionTexParamES(_recorder, pname)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glTexParameterfv', enabled=True, function_type=FuncType.PARAM, rec_condition='ConditionTexParamES(_recorder, pname)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glTexParameteri', enabled=True, function_type=FuncType.PARAM, rec_condition='ConditionTexParamES(_recorder, pname)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glTexParameteriv', enabled=True, function_type=FuncType.PARAM, rec_condition='ConditionTexParamES(_recorder, pname)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glTexParameterx', enabled=True, function_type=FuncType.PARAM, rec_condition='ConditionTexParamES(_recorder, pname)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfixed'),
    ],
)

function(name='glTexParameterxOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexParameterx')

function(name='glTexParameterxv', enabled=True, function_type=FuncType.PARAM, rec_condition='ConditionTexParamES(_recorder, pname)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfixed*', wrap_type='CGLfixed::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glTexParameterxvOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexParameterxv')

function(name='glTexRenderbufferNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbuffer'),
    ],
)

function(name='glTexStorage1D', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
    ],
)

function(name='glTexStorage1DEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexStorage1D')

function(name='glTexStorage2D', enabled=True, function_type=FuncType.PARAM, state_track=True, rec_condition='ConditionTextureES(_recorder)',
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

function(name='glTexStorage2DEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexStorage2D')

function(name='glTexStorage2DMultisample', enabled=True, function_type=FuncType.PARAM, state_track=True, rec_condition='ConditionTextureES(_recorder)',
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

function(name='glTexStorage3D', enabled=True, function_type=FuncType.PARAM, state_track=True, rec_condition='ConditionTextureES(_recorder)',
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

function(name='glTexStorage3DEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glTexStorage3D')

function(name='glTexStorage3DMultisample', enabled=True, function_type=FuncType.PARAM,
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

function(name='glTexStorage3DMultisampleOES', enabled=False, function_type=FuncType.NONE, inherit_from='glTexStorage3DMultisample')

function(name='glTexStorageMem1DEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glTexStorageMem2DEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glTexStorageMem2DMultisampleEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glTexStorageMem3DEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glTexStorageMem3DMultisampleEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glTexStorageSparseAMD', enabled=True, function_type=FuncType.PARAM,
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

function(name='glTexSubImage1D', enabled=True, function_type=FuncType.RESOURCE, rec_condition='ConditionTextureES(_recorder)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glTexSubImage1DEXT', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glTexSubImage1D')

function(name='glTexSubImage2D', enabled=True, function_type=FuncType.RESOURCE, rec_condition='ConditionTextureES(_recorder)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
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
function(name='glTexSubImage2DEXT', enabled=True, function_type=FuncType.RESOURCE, recorder_wrap=False, inherit_from='glTexSubImage2D')

function(name='glTexSubImage3D', enabled=True, function_type=FuncType.RESOURCE, rec_condition='ConditionTextureES(_recorder)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
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
function(name='glTexSubImage3DEXT', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glTexSubImage3D', recorder_wrap=False)

function(name='glTexSubImage3DOES', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glTexSubImage3D')

function(name='glTexSubImage4DSGIS', enabled=False, function_type=FuncType.NONE,
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

function(name='glTextureBarrier', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glTextureBarrierNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glTextureBarrier')

function(name='glTextureBuffer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

function(name='glTextureBufferEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

function(name='glTextureBufferRange', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
    ],
)

function(name='glTextureBufferRangeEXT', enabled=True, function_type=FuncType.PARAM,
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

function(name='glTextureColorMaskSGIS', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='red', type='GLboolean'),
        Argument(name='green', type='GLboolean'),
        Argument(name='blue', type='GLboolean'),
        Argument(name='alpha', type='GLboolean'),
    ],
)

function(name='glTextureFoveationParametersQCOM', enabled=False, function_type=FuncType.NONE,
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

function(name='glTextureImage1DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glTextureImage2DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glTextureImage2DMultisampleCoverageNV', enabled=True, function_type=FuncType.PARAM,
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

function(name='glTextureImage2DMultisampleNV', enabled=True, function_type=FuncType.PARAM,
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

function(name='glTextureImage3DEXT', enabled=True, function_type=FuncType.RESOURCE, state_track=True, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glTextureImage3DMultisampleCoverageNV', enabled=True, function_type=FuncType.PARAM,
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

function(name='glTextureImage3DMultisampleNV', enabled=True, function_type=FuncType.PARAM, state_track=True,
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

function(name='glTextureLightEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
    ],
)

function(name='glTextureMaterialEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='face', type='GLenum'),
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glTextureNormalEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
    ],
)

function(name='glTexturePageCommitmentEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glTextureParameterIiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glTextureParameterIivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glTextureParameterIuiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLuint*', wrap_type='CGLuint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glTextureParameterIuivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLuint*', wrap_type='CGLuint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glTextureParameterf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glTextureParameterfEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glTextureParameterfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, param'),
    ],
)

function(name='glTextureParameterfvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*', wrap_type='CGLfloat::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glTextureParameteri', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glTextureParameteriEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glTextureParameteriv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, param'),
    ],
)

function(name='glTextureParameterivEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*', wrap_type='CGLint::CSParamArray', wrap_params='pname, params'),
    ],
)

function(name='glTextureRangeAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='length', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glTextureRenderbufferEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='renderbuffer', type='GLuint', wrap_type='CGLRenderbufferEXT'),
    ],
)

function(name='glTextureStorage1D', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
    ],
)

function(name='glTextureStorage1DEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='target', type='GLenum'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
    ],
)

function(name='glTextureStorage2D', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='levels', type='GLsizei'),
        Argument(name='internalformat', type='GLenum'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

function(name='glTextureStorage2DEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
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

function(name='glTextureStorage2DMultisample', enabled=True, function_type=FuncType.PARAM, state_track=True,
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

function(name='glTextureStorage2DMultisampleEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
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

function(name='glTextureStorage3D', enabled=True, function_type=FuncType.PARAM, state_track=True,
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

function(name='glTextureStorage3DEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
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

function(name='glTextureStorage3DMultisample', enabled=True, function_type=FuncType.PARAM, state_track=True,
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

function(name='glTextureStorage3DMultisampleEXT', enabled=True, function_type=FuncType.PARAM,
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

function(name='glTextureStorageMem1DEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glTextureStorageMem2DEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glTextureStorageMem2DMultisampleEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glTextureStorageMem3DEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glTextureStorageMem3DMultisampleEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glTextureStorageSparseAMD', enabled=True, function_type=FuncType.PARAM,
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

function(name='glTextureSubImage1D', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glTextureSubImage1DEXT', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glTextureSubImage2D', enabled=True, function_type=FuncType.RESOURCE, rec_condition='ConditionTextureES(_recorder)', pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glTextureSubImage2DEXT', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glTextureSubImage3D', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glTextureSubImage3DEXT', enabled=True, function_type=FuncType.RESOURCE, pre_schedule='coherentBufferUpdate_PS(_recorder)',
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

function(name='glTextureView', enabled=True, function_type=FuncType.PARAM, state_track=True,
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

function(name='glTextureViewEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glTextureView')

function(name='glTextureViewOES', enabled=False, function_type=FuncType.NONE, inherit_from='glTextureView')

function(name='glTrackMatrixNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='target', type='GLenum'),
        Argument(name='address', type='GLuint'),
        Argument(name='matrix', type='GLenum'),
        Argument(name='transform', type='GLenum'),
    ],
)

function(name='glTransformFeedbackAttribsNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='count', type='GLsizei'),
        Argument(name='attribs', type='const GLint*'),
        Argument(name='bufferMode', type='GLenum'),
    ],
)

function(name='glTransformFeedbackBufferBase', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='xfb', type='GLuint'),
        Argument(name='index', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

function(name='glTransformFeedbackBufferRange', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='xfb', type='GLuint'),
        Argument(name='index', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='size', type='GLsizeiptr'),
    ],
)

function(name='glTransformFeedbackStreamAttribsNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='count', type='GLsizei'),
        Argument(name='attribs', type='const GLint*', wrap_params='count, attribs'),
        Argument(name='nbuffers', type='GLsizei'),
        Argument(name='bufstreams', type='const GLint*', wrap_params='nbuffers, bufstreams'),
        Argument(name='bufferMode', type='GLenum'),
    ],
)

function(name='glTransformFeedbackVaryings', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='count', type='GLsizei'),
        Argument(name='varyings', type='const GLchar*const*', wrap_type='CStringArray', wrap_params='varyings, count'),
        Argument(name='bufferMode', type='GLenum'),
    ],
)

function(name='glTransformFeedbackVaryingsEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glTransformFeedbackVaryings')

function(name='glTransformFeedbackVaryingsNV', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='locations', type='const GLint*'),
        Argument(name='bufferMode', type='GLenum'),
    ],
)

function(name='glTransformPathNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='resultPath', type='GLuint'),
        Argument(name='srcPath', type='GLuint'),
        Argument(name='transformType', type='GLenum'),
        Argument(name='transformValues', type='const GLfloat*', wrap_params='1, transformValues'),
    ],
)

function(name='glTranslated', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

function(name='glTranslatef', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

function(name='glTranslatex', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfixed'),
        Argument(name='y', type='GLfixed'),
        Argument(name='z', type='GLfixed'),
    ],
)

function(name='glTranslatexOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glTranslatex')

function(name='glUniform1d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLdouble'),
    ],
)

function(name='glUniform1dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLdouble*', wrap_params='count, value'),
    ],
)

function(name='glUniform1f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLfloat'),
    ],
)

function(name='glUniform1fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform1f')

function(name='glUniform1fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLfloat*', wrap_params='count, value'),
    ],
)

function(name='glUniform1fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform1fv')

function(name='glUniform1i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLint'),
    ],
)

function(name='glUniform1i64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLint64'),
    ],
)

function(name='glUniform1i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation', wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLint64EXT'),
    ],
)

function(name='glUniform1i64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64*', wrap_params='count, value'),
    ],
)

function(name='glUniform1i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64EXT*', wrap_params='count, value'),
    ],
)

function(name='glUniform1iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform1i')

function(name='glUniform1iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint*', wrap_params='count, value'),
    ],
)

function(name='glUniform1ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform1iv')

function(name='glUniform1ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLuint'),
    ],
)

function(name='glUniform1ui64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLuint64'),
    ],
)

function(name='glUniform1ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLuint64EXT'),
    ],
)

function(name='glUniform1ui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_params='count, value'),
    ],
)

function(name='glUniform1ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count, value'),
    ],
)

function(name='glUniform1uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform1ui')

function(name='glUniform1uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint*', wrap_params='count, value'),
    ],
)

function(name='glUniform1uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform1uiv')

function(name='glUniform2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
    ],
)

function(name='glUniform2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 2, value'),
    ],
)

function(name='glUniform2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLfloat'),
        Argument(name='v1', type='GLfloat'),
    ],
)

function(name='glUniform2fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform2f')

function(name='glUniform2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLfloat*', wrap_params='count* 2, value'),
    ],
)

function(name='glUniform2fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform2fv')

function(name='glUniform2i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLint'),
        Argument(name='v1', type='GLint'),
    ],
)

function(name='glUniform2i64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLint64'),
        Argument(name='y', type='GLint64'),
    ],
)

function(name='glUniform2i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLint64EXT'),
        Argument(name='y', type='GLint64EXT'),
    ],
)

function(name='glUniform2i64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64*', wrap_params='count * 2, value'),
    ],
)

function(name='glUniform2i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64EXT*', wrap_params='count * 2, value'),
    ],
)

function(name='glUniform2iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform2i')

function(name='glUniform2iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint*', wrap_params='count * 2, value'),
    ],
)

function(name='glUniform2ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform2iv')

function(name='glUniform2ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLuint'),
        Argument(name='v1', type='GLuint'),
    ],
)

function(name='glUniform2ui64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLuint64'),
        Argument(name='y', type='GLuint64'),
    ],
)

function(name='glUniform2ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLuint64EXT'),
        Argument(name='y', type='GLuint64EXT'),
    ],
)

function(name='glUniform2ui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_params='count * 2, value'),
    ],
)

function(name='glUniform2ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count * 2, value'),
    ],
)

function(name='glUniform2uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform2ui')

function(name='glUniform2uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint*', wrap_params='count * 2, value'),
    ],
)

function(name='glUniform2uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform2uiv')

function(name='glUniform3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

function(name='glUniform3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 3, value'),
    ],
)

function(name='glUniform3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLfloat'),
        Argument(name='v1', type='GLfloat'),
        Argument(name='v2', type='GLfloat'),
    ],
)

function(name='glUniform3fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform3f')

function(name='glUniform3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 3, value'),
    ],
)

function(name='glUniform3fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform3fv')

function(name='glUniform3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLint'),
        Argument(name='v1', type='GLint'),
        Argument(name='v2', type='GLint'),
    ],
)

function(name='glUniform3i64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLint64'),
        Argument(name='y', type='GLint64'),
        Argument(name='z', type='GLint64'),
    ],
)

function(name='glUniform3i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLint64EXT'),
        Argument(name='y', type='GLint64EXT'),
        Argument(name='z', type='GLint64EXT'),
    ],
)

function(name='glUniform3i64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64*', wrap_params='count * 3, value'),
    ],
)

function(name='glUniform3i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64EXT*', wrap_params='count * 3, value'),
    ],
)

function(name='glUniform3iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform3i')

function(name='glUniform3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint*', wrap_params='count * 3, value'),
    ],
)

function(name='glUniform3ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform3iv')

function(name='glUniform3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLuint'),
        Argument(name='v1', type='GLuint'),
        Argument(name='v2', type='GLuint'),
    ],
)

function(name='glUniform3ui64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLuint64'),
        Argument(name='y', type='GLuint64'),
        Argument(name='z', type='GLuint64'),
    ],
)

function(name='glUniform3ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLuint64EXT'),
        Argument(name='y', type='GLuint64EXT'),
        Argument(name='z', type='GLuint64EXT'),
    ],
)

function(name='glUniform3ui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_params='count * 3, value'),
    ],
)

function(name='glUniform3ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count * 3, value'),
    ],
)

function(name='glUniform3uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform3ui')

function(name='glUniform3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint*', wrap_params='count * 3, value'),
    ],
)

function(name='glUniform3uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform3uiv')

function(name='glUniform4d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

function(name='glUniform4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 4, value'),
    ],
)

function(name='glUniform4f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLfloat'),
        Argument(name='v1', type='GLfloat'),
        Argument(name='v2', type='GLfloat'),
        Argument(name='v3', type='GLfloat'),
    ],
)

function(name='glUniform4fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform4f')

function(name='glUniform4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 4, value'),
    ],
)

function(name='glUniform4fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform4fv')

function(name='glUniform4i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLint'),
        Argument(name='v1', type='GLint'),
        Argument(name='v2', type='GLint'),
        Argument(name='v3', type='GLint'),
    ],
)

function(name='glUniform4i64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLint64'),
        Argument(name='y', type='GLint64'),
        Argument(name='z', type='GLint64'),
        Argument(name='w', type='GLint64'),
    ],
)

function(name='glUniform4i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLint64EXT'),
        Argument(name='y', type='GLint64EXT'),
        Argument(name='z', type='GLint64EXT'),
        Argument(name='w', type='GLint64EXT'),
    ],
)

function(name='glUniform4i64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64*', wrap_params='count * 4, value'),
    ],
)

function(name='glUniform4i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint64EXT*', wrap_params='count * 4, value'),
    ],
)

function(name='glUniform4iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform4i')

function(name='glUniform4iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLint*', wrap_params='count * 4, value'),
    ],
)

function(name='glUniform4ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform4iv')

function(name='glUniform4ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='v0', type='GLuint'),
        Argument(name='v1', type='GLuint'),
        Argument(name='v2', type='GLuint'),
        Argument(name='v3', type='GLuint'),
    ],
)

function(name='glUniform4ui64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLuint64'),
        Argument(name='y', type='GLuint64'),
        Argument(name='z', type='GLuint64'),
        Argument(name='w', type='GLuint64'),
    ],
)

function(name='glUniform4ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='x', type='GLuint64EXT'),
        Argument(name='y', type='GLuint64EXT'),
        Argument(name='z', type='GLuint64EXT'),
        Argument(name='w', type='GLuint64EXT'),
    ],
)

function(name='glUniform4ui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_params='count * 4, value'),
    ],
)

function(name='glUniform4ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count * 4, value'),
    ],
)

function(name='glUniform4uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform4ui')

function(name='glUniform4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint*', wrap_params='count * 4, value'),
    ],
)

function(name='glUniform4uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniform4uiv')

function(name='glUniformBlockBinding', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='uniformBlockIndex', type='GLuint', wrap_type='CGLUniformBlockIndex', wrap_params='program, uniformBlockIndex'),
        Argument(name='uniformBlockBinding', type='GLuint'),
    ],
)

function(name='glUniformBufferEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

function(name='glUniformHandleui64ARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='value', type='GLuint64', wrap_type='CGLTextureHandle'),
    ],
)

function(name='glUniformHandleui64IMG', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint'),
        Argument(name='value', type='GLuint64'),
    ],
)

function(name='glUniformHandleui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='value', type='GLuint64'),
    ],
)

function(name='glUniformHandleui64vARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_type='CGLTextureHandle::CSArray', wrap_params='count, value'),
    ],
)

function(name='glUniformHandleui64vIMG', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*'),
    ],
)

function(name='glUniformHandleui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64*', wrap_params='count, value'),
    ],
)

function(name='glUniformMatrix2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 4, value'),
    ],
)

function(name='glUniformMatrix2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 4, value'),
    ],
)

function(name='glUniformMatrix2fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniformMatrix2fv')

function(name='glUniformMatrix2x3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 6, value'),
    ],
)

function(name='glUniformMatrix2x3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 6, value'),
    ],
)

function(name='glUniformMatrix2x3fvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glUniformMatrix2x3fv')

function(name='glUniformMatrix2x4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 8, value'),
    ],
)

function(name='glUniformMatrix2x4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 8, value'),
    ],
)

function(name='glUniformMatrix2x4fvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glUniformMatrix2x4fv')

function(name='glUniformMatrix3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 9, value'),
    ],
)

function(name='glUniformMatrix3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 9, value'),
    ],
)

function(name='glUniformMatrix3fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniformMatrix3fv')

function(name='glUniformMatrix3x2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 6, value'),
    ],
)

function(name='glUniformMatrix3x2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 6, value'),
    ],
)

function(name='glUniformMatrix3x2fvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glUniformMatrix3x2fv')

function(name='glUniformMatrix3x4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 12, value'),
    ],
)

function(name='glUniformMatrix3x4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 12, value'),
    ],
)

function(name='glUniformMatrix3x4fvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glUniformMatrix3x4fv')

function(name='glUniformMatrix4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 16, value'),
    ],
)

function(name='glUniformMatrix4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 16, value'),
    ],
)

function(name='glUniformMatrix4fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUniformMatrix4fv')

function(name='glUniformMatrix4x2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 8, value'),
    ],
)

function(name='glUniformMatrix4x2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 8, value'),
    ],
)

function(name='glUniformMatrix4x2fvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glUniformMatrix4x2fv')

function(name='glUniformMatrix4x3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLdouble*', wrap_params='count * 12, value'),
    ],
)

function(name='glUniformMatrix4x3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='transpose', type='GLboolean'),
        Argument(name='value', type='const GLfloat*', wrap_params='count * 12, value'),
    ],
)

function(name='glUniformMatrix4x3fvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glUniformMatrix4x3fv')

function(name='glUniformSubroutinesuiv', enabled=True, function_type=FuncType.PARAM, run_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='shadertype', type='GLenum'),
        Argument(name='count', type='GLsizei'),
        Argument(name='indices', type='const GLuint*', wrap_params='count, indices'),
    ],
)

function(name='glUniformui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='value', type='GLuint64EXT'),
    ],
)

function(name='glUniformui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='location', type='GLint', wrap_type='CGLUniformLocation' , wrap_params='current_program_tag, location'),
        Argument(name='count', type='GLsizei'),
        Argument(name='value', type='const GLuint64EXT*', wrap_params='count, value'),
    ],
)

function(name='glUnlockArraysEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glUnmapBuffer', enabled=True, function_type=FuncType.PARAM, state_track=True, pre_token='CgitsUnmapBuffer(target)', rec_condition='ConditionBufferES(_recorder)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='target', type='GLenum'),
    ],
)

function(name='glUnmapBufferARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glUnmapBuffer')

function(name='glUnmapBufferOES', enabled=True, function_type=FuncType.PARAM, inherit_from='glUnmapBuffer')

function(name='glUnmapNamedBuffer', enabled=True, function_type=FuncType.PARAM, state_track=True, pre_token='CgitsUnmapBuffer(0, buffer)', exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

function(name='glUnmapNamedBufferEXT', enabled=True, function_type=FuncType.PARAM, pre_token='CgitsUnmapBuffer(0, buffer)', inherit_from='glUnmapNamedBuffer')

function(name='glUnmapObjectBufferATI', enabled=True, function_type=FuncType.PARAM, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

function(name='glUnmapTexture2DINTEL', enabled=True, function_type=FuncType.PARAM, state_track=True, exec_post_recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='texture', type='GLuint', wrap_type='CGLTexture'),
        Argument(name='level', type='GLint'),
    ],
)

function(name='glUpdateObjectBufferATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLuint'),
        Argument(name='size', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
        Argument(name='preserve', type='GLenum'),
    ],
)

function(name='glUseProgram', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

function(name='glUseProgramObjectARB', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='programObj', type='GLhandleARB', wrap_type='CGLProgram'),
    ],
)

function(name='glUseProgramStages', enabled=True, function_type=FuncType.BIND, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pipeline', type='GLuint', wrap_type='CGLPipeline'),
        Argument(name='stages', type='GLbitfield'),
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

function(name='glUseProgramStagesEXT', enabled=True, function_type=FuncType.BIND, inherit_from='glUseProgramStages')

function(name='glUseShaderProgramEXT', enabled=True, function_type=FuncType.BIND,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

function(name='glVDPAUFiniNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[],
)

function(name='glVDPAUGetSurfaceivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='surface', type='GLvdpauSurfaceNV'),
        Argument(name='pname', type='GLenum'),
        Argument(name='bufSize', type='GLsizei'),
        Argument(name='length', type='GLsizei*'),
        Argument(name='values', type='GLint*'),
    ],
)

function(name='glVDPAUInitNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vdpDevice', type='const void*'),
        Argument(name='getProcAddress', type='const void*'),
    ],
)

function(name='glVDPAUIsSurfaceNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLboolean'),
    args=[
        Argument(name='surface', type='GLvdpauSurfaceNV'),
    ],
)

function(name='glVDPAUMapSurfacesNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='numSurfaces', type='GLsizei'),
        Argument(name='surfaces', type='const GLvdpauSurfaceNV*'),
    ],
)

function(name='glVDPAURegisterOutputSurfaceNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLvdpauSurfaceNV'),
    args=[
        Argument(name='vdpSurface', type='const void*'),
        Argument(name='target', type='GLenum'),
        Argument(name='numTextureNames', type='GLsizei'),
        Argument(name='textureNames', type='const GLuint*'),
    ],
)

function(name='glVDPAURegisterVideoSurfaceNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLvdpauSurfaceNV'),
    args=[
        Argument(name='vdpSurface', type='const void*'),
        Argument(name='target', type='GLenum'),
        Argument(name='numTextureNames', type='GLsizei'),
        Argument(name='textureNames', type='const GLuint*'),
    ],
)

function(name='glVDPAUSurfaceAccessNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='surface', type='GLvdpauSurfaceNV'),
        Argument(name='access', type='GLenum'),
    ],
)

function(name='glVDPAUUnmapSurfacesNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='numSurface', type='GLsizei'),
        Argument(name='surfaces', type='const GLvdpauSurfaceNV*'),
    ],
)

function(name='glVDPAUUnregisterSurfaceNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='surface', type='GLvdpauSurfaceNV'),
    ],
)

function(name='glValidateProgram', enabled=True, function_type=FuncType.RESOURCE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='program', type='GLuint', wrap_type='CGLProgram'),
    ],
)

function(name='glValidateProgramARB', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glValidateProgram',
    args=[
        Argument(name='programObj', type='GLhandleARB', wrap_type='CGLProgram'),
    ],
)

function(name='glValidateProgramPipeline', enabled=True, function_type=FuncType.RESOURCE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pipeline', type='GLuint', wrap_type='CGLPipeline'),
    ],
)

function(name='glValidateProgramPipelineEXT', enabled=True, function_type=FuncType.RESOURCE, inherit_from='glValidateProgramPipeline')

function(name='glVariantArrayObjectATI', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLuint'),
    ],
)

function(name='glVariantPointerEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLuint'),
        Argument(name='addr', type='const void*'),
    ],
)

function(name='glVariantbvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='addr', type='const GLbyte*'),
    ],
)

function(name='glVariantdvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='addr', type='const GLdouble*'),
    ],
)

function(name='glVariantfvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='addr', type='const GLfloat*'),
    ],
)

function(name='glVariantivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='addr', type='const GLint*'),
    ],
)

function(name='glVariantsvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='addr', type='const GLshort*'),
    ],
)

function(name='glVariantubvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='addr', type='const GLubyte*'),
    ],
)

function(name='glVariantuivEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='addr', type='const GLuint*'),
    ],
)

function(name='glVariantusvEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='id', type='GLuint'),
        Argument(name='addr', type='const GLushort*'),
    ],
)

function(name='glVertex2bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLbyte'),
        Argument(name='y', type='GLbyte'),
    ],
)

function(name='glVertex2bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLbyte*'),
    ],
)

function(name='glVertex2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
    ],
)

function(name='glVertex2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='2, v'),
    ],
)

function(name='glVertex2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
    ],
)

function(name='glVertex2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='2, v'),
    ],
)

function(name='glVertex2hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLhalfNV'),
        Argument(name='y', type='GLhalfNV'),
    ],
)

function(name='glVertex2hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='2, v'),
    ],
)

function(name='glVertex2i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
    ],
)

function(name='glVertex2iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='2, v'),
    ],
)

function(name='glVertex2s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
    ],
)

function(name='glVertex2sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='2, v'),
    ],
)

function(name='glVertex2xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfixed'),
    ],
)

function(name='glVertex2xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

function(name='glVertex3bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLbyte'),
        Argument(name='y', type='GLbyte'),
        Argument(name='z', type='GLbyte'),
    ],
)

function(name='glVertex3bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLbyte*'),
    ],
)

function(name='glVertex3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

function(name='glVertex3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

function(name='glVertex3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

function(name='glVertex3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

function(name='glVertex3hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLhalfNV'),
        Argument(name='y', type='GLhalfNV'),
        Argument(name='z', type='GLhalfNV'),
    ],
)

function(name='glVertex3hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='3, v'),
    ],
)

function(name='glVertex3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
    ],
)

function(name='glVertex3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

function(name='glVertex3s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
    ],
)

function(name='glVertex3sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

function(name='glVertex3xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfixed'),
        Argument(name='y', type='GLfixed'),
    ],
)

function(name='glVertex3xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

function(name='glVertex4bOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLbyte'),
        Argument(name='y', type='GLbyte'),
        Argument(name='z', type='GLbyte'),
        Argument(name='w', type='GLbyte'),
    ],
)

function(name='glVertex4bvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLbyte*'),
    ],
)

function(name='glVertex4d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

function(name='glVertex4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='4, v'),
    ],
)

function(name='glVertex4f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
    ],
)

function(name='glVertex4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

function(name='glVertex4hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLhalfNV'),
        Argument(name='y', type='GLhalfNV'),
        Argument(name='z', type='GLhalfNV'),
        Argument(name='w', type='GLhalfNV'),
    ],
)

function(name='glVertex4hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLhalfNV*', wrap_params='4, v'),
    ],
)

function(name='glVertex4i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
        Argument(name='w', type='GLint'),
    ],
)

function(name='glVertex4iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

function(name='glVertex4s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
        Argument(name='w', type='GLshort'),
    ],
)

function(name='glVertex4sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='4, v'),
    ],
)

function(name='glVertex4xOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfixed'),
        Argument(name='y', type='GLfixed'),
        Argument(name='z', type='GLfixed'),
    ],
)

function(name='glVertex4xvOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='coords', type='const GLfixed*'),
    ],
)

function(name='glVertexArrayAttribBinding', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='attribindex', type='GLuint'),
        Argument(name='bindingindex', type='GLuint'),
    ],
)

function(name='glVertexArrayAttribFormat', enabled=True, function_type=FuncType.PARAM, state_track=True,
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

function(name='glVertexArrayAttribIFormat', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='attribindex', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='relativeoffset', type='GLuint'),
    ],
)

function(name='glVertexArrayAttribLFormat', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='attribindex', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='relativeoffset', type='GLuint'),
    ],
)

function(name='glVertexArrayBindVertexBufferEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='bindingindex', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='stride', type='GLsizei'),
    ],
)

function(name='glVertexArrayBindingDivisor', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='bindingindex', type='GLuint'),
        Argument(name='divisor', type='GLuint'),
    ],
)

function(name='glVertexArrayColorOffsetEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glVertexArrayEdgeFlagOffsetEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='offset', type='GLintptr'),
    ],
)

function(name='glVertexArrayElementBuffer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
    ],
)

function(name='glVertexArrayFogCoordOffsetEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='offset', type='GLintptr'),
    ],
)

function(name='glVertexArrayIndexOffsetEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='offset', type='GLintptr'),
    ],
)

function(name='glVertexArrayMultiTexCoordOffsetEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glVertexArrayNormalOffsetEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='offset', type='GLintptr'),
    ],
)

function(name='glVertexArrayParameteriAPPLE', enabled=False, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glVertexArrayRangeAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='length', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glVertexArrayRangeNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='length', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glVertexArraySecondaryColorOffsetEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glVertexArrayTexCoordOffsetEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glVertexArrayVertexAttribBindingEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='attribindex', type='GLuint'),
        Argument(name='bindingindex', type='GLuint'),
    ],
)

function(name='glVertexArrayVertexAttribDivisorEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='index', type='GLuint'),
        Argument(name='divisor', type='GLuint'),
    ],
)

function(name='glVertexArrayVertexAttribFormatEXT', enabled=True, function_type=FuncType.PARAM,
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

function(name='glVertexArrayVertexAttribIFormatEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='attribindex', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='relativeoffset', type='GLuint'),
    ],
)

function(name='glVertexArrayVertexAttribIOffsetEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glVertexArrayVertexAttribLFormatEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='attribindex', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='relativeoffset', type='GLuint'),
    ],
)

function(name='glVertexArrayVertexAttribLOffsetEXT', enabled=True, function_type=FuncType.PARAM,
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

function(name='glVertexArrayVertexAttribOffsetEXT', enabled=True, function_type=FuncType.PARAM,
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

function(name='glVertexArrayVertexBindingDivisorEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='bindingindex', type='GLuint'),
        Argument(name='divisor', type='GLuint'),
    ],
)

function(name='glVertexArrayVertexBuffer', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vaobj', type='GLuint'),
        Argument(name='bindingindex', type='GLuint'),
        Argument(name='buffer', type='GLuint', wrap_type='CGLBuffer'),
        Argument(name='offset', type='GLintptr'),
        Argument(name='stride', type='GLsizei'),
    ],
)

function(name='glVertexArrayVertexBuffers', enabled=False, function_type=FuncType.NONE,
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

function(name='glVertexArrayVertexOffsetEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glVertexAttrib1d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
    ],
)

function(name='glVertexAttrib1dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1d')

function(name='glVertexAttrib1dNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1d')

function(name='glVertexAttrib1dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLdouble*', wrap_params='1, v'),
    ],
)

function(name='glVertexAttrib1dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1dv')

function(name='glVertexAttrib1dvNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1dv')

function(name='glVertexAttrib1f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLfloat'),
    ],
)

function(name='glVertexAttrib1fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1f')

function(name='glVertexAttrib1fNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1f')

function(name='glVertexAttrib1fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLfloat*', wrap_params='1, v'),
    ],
)

function(name='glVertexAttrib1fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1fv')

function(name='glVertexAttrib1fvNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1fv')

function(name='glVertexAttrib1hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLhalfNV'),
    ],
)

function(name='glVertexAttrib1hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLhalfNV*', wrap_params='1, v'),
    ],
)

function(name='glVertexAttrib1s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLshort'),
    ],
)

function(name='glVertexAttrib1sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1s')

function(name='glVertexAttrib1sNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1s')

function(name='glVertexAttrib1sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLshort*', wrap_params='1, v'),
    ],
)

function(name='glVertexAttrib1svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1sv')

function(name='glVertexAttrib1svNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib1sv')

function(name='glVertexAttrib2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
    ],
)

function(name='glVertexAttrib2dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2d')

function(name='glVertexAttrib2dNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2d')

function(name='glVertexAttrib2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLdouble*', wrap_params='2, v'),
    ],
)

function(name='glVertexAttrib2dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2dv')

function(name='glVertexAttrib2dvNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2dv')

function(name='glVertexAttrib2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
    ],
)

function(name='glVertexAttrib2fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2f')

function(name='glVertexAttrib2fNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2f')

function(name='glVertexAttrib2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLfloat*', wrap_params='2, v'),
    ],
)

function(name='glVertexAttrib2fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2fv')

function(name='glVertexAttrib2fvNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2fv')

function(name='glVertexAttrib2hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLhalfNV'),
        Argument(name='y', type='GLhalfNV'),
    ],
)

function(name='glVertexAttrib2hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLhalfNV*', wrap_params='2, v'),
    ],
)

function(name='glVertexAttrib2s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
    ],
)

function(name='glVertexAttrib2sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2s')

function(name='glVertexAttrib2sNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2s')

function(name='glVertexAttrib2sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLshort*', wrap_params='2, v'),
    ],
)

function(name='glVertexAttrib2svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2sv')

function(name='glVertexAttrib2svNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib2sv')

function(name='glVertexAttrib3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

function(name='glVertexAttrib3dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3d')

function(name='glVertexAttrib3dNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3d')

function(name='glVertexAttrib3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

function(name='glVertexAttrib3dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3dv')

function(name='glVertexAttrib3dvNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3dv')

function(name='glVertexAttrib3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

function(name='glVertexAttrib3fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3f')

function(name='glVertexAttrib3fNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3f')

function(name='glVertexAttrib3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

function(name='glVertexAttrib3fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3fv')

function(name='glVertexAttrib3fvNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3fv')

function(name='glVertexAttrib3hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLhalfNV'),
        Argument(name='y', type='GLhalfNV'),
        Argument(name='z', type='GLhalfNV'),
    ],
)

function(name='glVertexAttrib3hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLhalfNV*', wrap_params='3, v'),
    ],
)

function(name='glVertexAttrib3s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
    ],
)

function(name='glVertexAttrib3sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3s')

function(name='glVertexAttrib3sNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3s')

function(name='glVertexAttrib3sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

function(name='glVertexAttrib3svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3sv')

function(name='glVertexAttrib3svNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib3sv')

function(name='glVertexAttrib4Nbv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLbyte*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttrib4NbvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4Nbv')

function(name='glVertexAttrib4Niv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttrib4NivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4Niv')

function(name='glVertexAttrib4Nsv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLshort*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttrib4NsvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4Nsv')

function(name='glVertexAttrib4Nub', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLubyte'),
        Argument(name='y', type='GLubyte'),
        Argument(name='z', type='GLubyte'),
        Argument(name='w', type='GLubyte'),
    ],
)

function(name='glVertexAttrib4NubARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4Nub')

function(name='glVertexAttrib4Nubv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLubyte*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttrib4NubvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4Nubv')

function(name='glVertexAttrib4Nuiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttrib4NuivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4Nuiv')

function(name='glVertexAttrib4Nusv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLushort*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttrib4NusvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4Nusv')

function(name='glVertexAttrib4bv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLbyte*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttrib4bvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4bv')

function(name='glVertexAttrib4d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

function(name='glVertexAttrib4dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4d')

function(name='glVertexAttrib4dNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4d')

function(name='glVertexAttrib4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLdouble*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttrib4dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4dv')

function(name='glVertexAttrib4dvNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4dv')

function(name='glVertexAttrib4f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
    ],
)

function(name='glVertexAttrib4fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4f')

function(name='glVertexAttrib4fNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4f')

function(name='glVertexAttrib4fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttrib4fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4fv')

function(name='glVertexAttrib4fvNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4fv')

function(name='glVertexAttrib4hNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLhalfNV'),
        Argument(name='y', type='GLhalfNV'),
        Argument(name='z', type='GLhalfNV'),
        Argument(name='w', type='GLhalfNV'),
    ],
)

function(name='glVertexAttrib4hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLhalfNV*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttrib4iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttrib4ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4iv')

function(name='glVertexAttrib4s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
        Argument(name='w', type='GLshort'),
    ],
)

function(name='glVertexAttrib4sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4s')

function(name='glVertexAttrib4sNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4s')

function(name='glVertexAttrib4sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLshort*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttrib4svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4sv')

function(name='glVertexAttrib4svNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4sv')

function(name='glVertexAttrib4ubNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLubyte'),
        Argument(name='y', type='GLubyte'),
        Argument(name='z', type='GLubyte'),
        Argument(name='w', type='GLubyte'),
    ],
)

function(name='glVertexAttrib4ubv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLubyte*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttrib4ubvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4ubv')

function(name='glVertexAttrib4ubvNV', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4ubv')

function(name='glVertexAttrib4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttrib4uivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4uiv')

function(name='glVertexAttrib4usv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLushort*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttrib4usvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttrib4usv')

function(name='glVertexAttribArrayObjectATI', enabled=True, function_type=FuncType.PARAM,
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

function(name='glVertexAttribBinding', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='attribindex', type='GLuint'),
        Argument(name='bindingindex', type='GLuint'),
    ],
)

function(name='glVertexAttribDivisor', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='divisor', type='GLuint'),
    ],
)

function(name='glVertexAttribDivisorANGLE', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribDivisor', run_wrap=False)

function(name='glVertexAttribDivisorARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribDivisor', run_wrap=False)

function(name='glVertexAttribDivisorEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glVertexAttribDivisor', run_wrap=False)

function(name='glVertexAttribDivisorNV', enabled=False, function_type=FuncType.NONE, inherit_from='glVertexAttribDivisor', run_wrap=False)

function(name='glVertexAttribFormat', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='attribindex', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='relativeoffset', type='GLuint'),
    ],
)

function(name='glVertexAttribFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='stride', type='GLsizei'),
    ],
)

function(name='glVertexAttribI1i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint'),
    ],
)

function(name='glVertexAttribI1iEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI1i')

function(name='glVertexAttribI1iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint*', wrap_params='1, v'),
    ],
)

function(name='glVertexAttribI1ivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI1iv')

function(name='glVertexAttribI1ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint'),
    ],
)

function(name='glVertexAttribI1uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI1ui')

function(name='glVertexAttribI1uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint*', wrap_params='1, v'),
    ],
)

function(name='glVertexAttribI1uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI1uiv')

function(name='glVertexAttribI2i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
    ],
)

function(name='glVertexAttribI2iEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI2i')

function(name='glVertexAttribI2iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint*', wrap_params='2, v'),
    ],
)

function(name='glVertexAttribI2ivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI2iv')

function(name='glVertexAttribI2ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint'),
        Argument(name='y', type='GLuint'),
    ],
)

function(name='glVertexAttribI2uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI2ui')

function(name='glVertexAttribI2uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint*', wrap_params='2, v'),
    ],
)

function(name='glVertexAttribI2uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI2uiv')

function(name='glVertexAttribI3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
    ],
)

function(name='glVertexAttribI3iEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI3i')

function(name='glVertexAttribI3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

function(name='glVertexAttribI3ivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI3iv')

function(name='glVertexAttribI3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint'),
        Argument(name='y', type='GLuint'),
        Argument(name='z', type='GLuint'),
    ],
)

function(name='glVertexAttribI3uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI3ui')

function(name='glVertexAttribI3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint*', wrap_params='3, v'),
    ],
)

function(name='glVertexAttribI3uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI3uiv')

function(name='glVertexAttribI4bv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLbyte*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttribI4bvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI4bv')

function(name='glVertexAttribI4i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
        Argument(name='w', type='GLint'),
    ],
)

function(name='glVertexAttribI4iEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI4i')

function(name='glVertexAttribI4iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttribI4ivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI4iv')

function(name='glVertexAttribI4sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLshort*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttribI4svEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI4sv')

function(name='glVertexAttribI4ubv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLubyte*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttribI4ubvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI4ubv')

function(name='glVertexAttribI4ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint'),
        Argument(name='y', type='GLuint'),
        Argument(name='z', type='GLuint'),
        Argument(name='w', type='GLuint'),
    ],
)

function(name='glVertexAttribI4uiEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI4ui')

function(name='glVertexAttribI4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttribI4uivEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI4uiv')

function(name='glVertexAttribI4usv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLushort*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttribI4usvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribI4usv')

function(name='glVertexAttribIFormat', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='attribindex', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='relativeoffset', type='GLuint'),
    ],
)

function(name='glVertexAttribIFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
    ],
)

function(name='glVertexAttribIPointer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

function(name='glVertexAttribIPointerEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribIPointer')

function(name='glVertexAttribL1d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
    ],
)

function(name='glVertexAttribL1dEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribL1d')

function(name='glVertexAttribL1dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLdouble*', wrap_params='1, v'),
    ],
)

function(name='glVertexAttribL1dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribL1dv')

function(name='glVertexAttribL1i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint64EXT'),
    ],
)

function(name='glVertexAttribL1i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint64EXT*', wrap_params='1, v'),
    ],
)

function(name='glVertexAttribL1ui64ARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint64EXT'),
    ],
)

function(name='glVertexAttribL1ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint64EXT'),
    ],
)

function(name='glVertexAttribL1ui64vARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint64EXT*'),
    ],
)

function(name='glVertexAttribL1ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint64EXT*', wrap_params='1, v'),
    ],
)

function(name='glVertexAttribL2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
    ],
)

function(name='glVertexAttribL2dEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribL2d')

function(name='glVertexAttribL2dv', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLdouble*', wrap_params='2, v'),
    ],
)

function(name='glVertexAttribL2dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribL2dv')

function(name='glVertexAttribL2i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint64EXT'),
        Argument(name='y', type='GLint64EXT'),
    ],
)

function(name='glVertexAttribL2i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint64EXT*', wrap_params='2, v'),
    ],
)

function(name='glVertexAttribL2ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint64EXT'),
        Argument(name='y', type='GLuint64EXT'),
    ],
)

function(name='glVertexAttribL2ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint64EXT*', wrap_params='2, v'),
    ],
)

function(name='glVertexAttribL3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

function(name='glVertexAttribL3dEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribL3d')

function(name='glVertexAttribL3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

function(name='glVertexAttribL3dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribL3dv')

function(name='glVertexAttribL3i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint64EXT'),
        Argument(name='y', type='GLint64EXT'),
        Argument(name='z', type='GLint64EXT'),
    ],
)

function(name='glVertexAttribL3i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint64EXT*', wrap_params='3, v'),
    ],
)

function(name='glVertexAttribL3ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint64EXT'),
        Argument(name='y', type='GLuint64EXT'),
        Argument(name='z', type='GLuint64EXT'),
    ],
)

function(name='glVertexAttribL3ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint64EXT*', wrap_params='3, v'),
    ],
)

function(name='glVertexAttribL4d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

function(name='glVertexAttribL4dEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribL4d')

function(name='glVertexAttribL4dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLdouble*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttribL4dvEXT', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribL4dv')

function(name='glVertexAttribL4i64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLint64EXT'),
        Argument(name='y', type='GLint64EXT'),
        Argument(name='z', type='GLint64EXT'),
        Argument(name='w', type='GLint64EXT'),
    ],
)

function(name='glVertexAttribL4i64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLint64EXT*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttribL4ui64NV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLuint64EXT'),
        Argument(name='y', type='GLuint64EXT'),
        Argument(name='z', type='GLuint64EXT'),
        Argument(name='w', type='GLuint64EXT'),
    ],
)

function(name='glVertexAttribL4ui64vNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLuint64EXT*', wrap_params='4, v'),
    ],
)

function(name='glVertexAttribLFormat', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='attribindex', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='relativeoffset', type='GLuint'),
    ],
)

function(name='glVertexAttribLFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
    ],
)

function(name='glVertexAttribLPointer', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glVertexAttribLPointerEXT', enabled=False, function_type=FuncType.NONE, inherit_from='glVertexAttribLPointer')

function(name='glVertexAttribP1ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='value', type='GLuint'),
    ],
)

function(name='glVertexAttribP1uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='value', type='const GLuint*', wrap_params='1, value'),
    ],
)

function(name='glVertexAttribP2ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='value', type='GLuint'),
    ],
)

function(name='glVertexAttribP2uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='value', type='const GLuint*', wrap_params='2, value'),
    ],
)

function(name='glVertexAttribP3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='value', type='GLuint'),
    ],
)

function(name='glVertexAttribP3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='value', type='const GLuint*', wrap_params='3, value'),
    ],
)

function(name='glVertexAttribP4ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='value', type='GLuint'),
    ],
)

function(name='glVertexAttribP4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='type', type='GLenum'),
        Argument(name='normalized', type='GLboolean'),
        Argument(name='value', type='const GLuint*', wrap_params='4, value'),
    ],
)

function(name='glVertexAttribParameteriAMD', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glVertexAttribPointer', enabled=True, function_type=FuncType.PARAM, state_track=True,
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

function(name='glVertexAttribPointerARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glVertexAttribPointer')

function(name='glVertexAttribPointerNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='fsize', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glVertexAttribs1dvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLdouble*', wrap_params='count, v'),
    ],
)

function(name='glVertexAttribs1fvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*', wrap_params='count, v'),
    ],
)

function(name='glVertexAttribs1hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='n', type='GLsizei'),
        Argument(name='v', type='const GLhalfNV*', wrap_params='n, v'),
    ],
)

function(name='glVertexAttribs1svNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLshort*', wrap_params='count, v'),
    ],
)

function(name='glVertexAttribs2dvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLdouble*', wrap_params='2 * count, v'),
    ],
)

function(name='glVertexAttribs2fvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*', wrap_params='2 * count, v'),
    ],
)

function(name='glVertexAttribs2hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='n', type='GLsizei'),
        Argument(name='v', type='const GLhalfNV*', wrap_params='2 * n, v'),
    ],
)

function(name='glVertexAttribs2svNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLshort*', wrap_params='2 * count, v'),
    ],
)

function(name='glVertexAttribs3dvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLdouble*', wrap_params='3 * count, v'),
    ],
)

function(name='glVertexAttribs3fvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*', wrap_params='3 * count, v'),
    ],
)

function(name='glVertexAttribs3hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='n', type='GLsizei'),
        Argument(name='v', type='const GLhalfNV*', wrap_params='3 * n, v'),
    ],
)

function(name='glVertexAttribs3svNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLshort*', wrap_params='3 * count, v'),
    ],
)

function(name='glVertexAttribs4dvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLdouble*', wrap_params='4 * count, v'),
    ],
)

function(name='glVertexAttribs4fvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*', wrap_params='4 * count, v'),
    ],
)

function(name='glVertexAttribs4hvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='n', type='GLsizei'),
        Argument(name='v', type='const GLhalfNV*', wrap_params='4 * n, v'),
    ],
)

function(name='glVertexAttribs4svNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLshort*', wrap_params='4 * count, v'),
    ],
)

function(name='glVertexAttribs4ubvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLubyte*', wrap_params='4 * count, v'),
    ],
)

function(name='glVertexBindingDivisor', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='bindingindex', type='GLuint'),
        Argument(name='divisor', type='GLuint'),
    ],
)

function(name='glVertexBlendARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='count', type='GLint'),
    ],
)

function(name='glVertexBlendEnvfATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLfloat'),
    ],
)

function(name='glVertexBlendEnviATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='pname', type='GLenum'),
        Argument(name='param', type='GLint'),
    ],
)

function(name='glVertexFormatNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
    ],
)

function(name='glVertexP2ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='value', type='GLuint'),
    ],
)

function(name='glVertexP2uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='value', type='const GLuint*', wrap_params='1, value'),
    ],
)

function(name='glVertexP3ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='value', type='GLuint'),
    ],
)

function(name='glVertexP3uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='value', type='const GLuint*', wrap_params='1, value'),
    ],
)

function(name='glVertexP4ui', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='value', type='GLuint'),
    ],
)

function(name='glVertexP4uiv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='type', type='GLenum'),
        Argument(name='value', type='const GLuint*', wrap_params='1, value'),
    ],
)

function(name='glVertexPointSizefAPPLE', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLfloat'),
    ],
)

function(name='glVertexPointer', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

function(name='glVertexPointerBounds', enabled=True, function_type=FuncType.PARAM, run_wrap=True, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const GLvoid*', wrap_type='CAttribPtr'),
        Argument(name='count', type='GLsizei'),
    ],
)

function(name='glVertexPointerEXT', enabled=True, function_type=FuncType.PARAM, state_track=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='count', type='GLsizei'),
        Argument(name='pointer', type='const void*', wrap_type='CAttribPtr'),
    ],
)

function(name='glVertexPointerListIBM', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLint'),
        Argument(name='pointer', type='const void**'),
        Argument(name='ptrstride', type='GLint'),
    ],
)

function(name='glVertexPointervINTEL', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='pointer', type='const void**'),
    ],
)

function(name='glVertexStream1dATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLdouble'),
    ],
)

function(name='glVertexStream1dvATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLdouble*', wrap_params='1, coords'),
    ],
)

function(name='glVertexStream1fATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLfloat'),
    ],
)

function(name='glVertexStream1fvATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLfloat*', wrap_params='1, coords'),
    ],
)

function(name='glVertexStream1iATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLint'),
    ],
)

function(name='glVertexStream1ivATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLint*', wrap_params='1, coords'),
    ],
)

function(name='glVertexStream1sATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLshort'),
    ],
)

function(name='glVertexStream1svATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLshort*', wrap_params='1, coords'),
    ],
)

function(name='glVertexStream2dATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
    ],
)

function(name='glVertexStream2dvATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLdouble*', wrap_params='2, coords'),
    ],
)

function(name='glVertexStream2fATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
    ],
)

function(name='glVertexStream2fvATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLfloat*', wrap_params='2, coords'),
    ],
)

function(name='glVertexStream2iATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
    ],
)

function(name='glVertexStream2ivATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLint*', wrap_params='2, coords'),
    ],
)

function(name='glVertexStream2sATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
    ],
)

function(name='glVertexStream2svATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLshort*', wrap_params='2, coords'),
    ],
)

function(name='glVertexStream3dATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

function(name='glVertexStream3dvATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLdouble*', wrap_params='3, coords'),
    ],
)

function(name='glVertexStream3fATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

function(name='glVertexStream3fvATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLfloat*', wrap_params='3, coords'),
    ],
)

function(name='glVertexStream3iATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
    ],
)

function(name='glVertexStream3ivATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLint*', wrap_params='3, coords'),
    ],
)

function(name='glVertexStream3sATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
    ],
)

function(name='glVertexStream3svATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLshort*', wrap_params='3, coords'),
    ],
)

function(name='glVertexStream4dATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

function(name='glVertexStream4dvATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLdouble*', wrap_params='4, coords'),
    ],
)

function(name='glVertexStream4fATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
    ],
)

function(name='glVertexStream4fvATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLfloat*', wrap_params='4, coords'),
    ],
)

function(name='glVertexStream4iATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
        Argument(name='w', type='GLint'),
    ],
)

function(name='glVertexStream4ivATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLint*', wrap_params='4, coords'),
    ],
)

function(name='glVertexStream4sATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
        Argument(name='w', type='GLshort'),
    ],
)

function(name='glVertexStream4svATI', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='stream', type='GLenum'),
        Argument(name='coords', type='const GLshort*', wrap_params='4, coords'),
    ],
)

function(name='glVertexWeightPointerEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glVertexWeightfEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='weight', type='GLfloat'),
    ],
)

function(name='glVertexWeightfvEXT', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='weight', type='const GLfloat*', wrap_params='1, weight'),
    ],
)

function(name='glVertexWeighthNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='weight', type='GLhalfNV'),
    ],
)

function(name='glVertexWeighthvNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='weight', type='const GLhalfNV*', wrap_params='1, weight'),
    ],
)

function(name='glVideoCaptureNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='GLenum'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='sequence_num', type='GLuint*'),
        Argument(name='capture_time', type='GLuint64EXT*'),
    ],
)

function(name='glVideoCaptureStreamParameterdvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='stream', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLdouble*'),
    ],
)

function(name='glVideoCaptureStreamParameterfvNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='stream', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLfloat*'),
    ],
)

function(name='glVideoCaptureStreamParameterivNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='video_capture_slot', type='GLuint'),
        Argument(name='stream', type='GLuint'),
        Argument(name='pname', type='GLenum'),
        Argument(name='params', type='const GLint*'),
    ],
)

function(name='glViewport', enabled=True, function_type=FuncType.PARAM, run_wrap=True, pre_token='CgitsViewportSettings(x, y, width, height)', recorder_wrap=True,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='width', type='GLsizei'),
        Argument(name='height', type='GLsizei'),
    ],
)

function(name='glViewportArrayv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='first', type='GLuint'),
        Argument(name='count', type='GLsizei'),
        Argument(name='v', type='const GLfloat*', wrap_params='4 * count, v'),
    ],
)

function(name='glViewportArrayvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glViewportArrayv')

function(name='glViewportArrayvOES', enabled=False, function_type=FuncType.NONE, inherit_from='glViewportArrayv')

function(name='glViewportIndexedf', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
        Argument(name='h', type='GLfloat'),
    ],
)

function(name='glViewportIndexedfNV', enabled=False, function_type=FuncType.NONE, inherit_from='glViewportIndexedf')

function(name='glViewportIndexedfOES', enabled=False, function_type=FuncType.NONE, inherit_from='glViewportIndexedf')

function(name='glViewportIndexedfv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

function(name='glViewportIndexedfvNV', enabled=False, function_type=FuncType.NONE, inherit_from='glViewportIndexedfv')

function(name='glViewportIndexedfvOES', enabled=False, function_type=FuncType.NONE, inherit_from='glViewportIndexedfv')

function(name='glViewportPositionWScaleNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='xcoeff', type='GLfloat'),
        Argument(name='ycoeff', type='GLfloat'),
    ],
)

function(name='glViewportSwizzleNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='index', type='GLuint'),
        Argument(name='swizzlex', type='GLenum'),
        Argument(name='swizzley', type='GLenum'),
        Argument(name='swizzlez', type='GLenum'),
        Argument(name='swizzlew', type='GLenum'),
    ],
)

function(name='glWaitSemaphoreEXT', enabled=False, function_type=FuncType.NONE,
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

function(name='glWaitSync', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='sync', type='GLsync', wrap_type='CGLsync'),
        Argument(name='flags', type='GLbitfield'),
        Argument(name='timeout', type='GLuint64'),
    ],
)

function(name='glWaitSyncAPPLE', enabled=False, function_type=FuncType.PARAM, inherit_from='glWaitSync')

function(name='glWaitVkSemaphoreNV', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='vkSemaphore', type='GLuint64'),
    ],
)

function(name='glWeightPathsNV', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='resultPath', type='GLuint'),
        Argument(name='numPaths', type='GLsizei'),
        Argument(name='paths', type='const GLuint*', wrap_params='1, paths'),
        Argument(name='weights', type='const GLfloat*', wrap_params='1, weights'),
    ],
)

function(name='glWeightPointerARB', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glWeightPointerOES', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const void*'),
    ],
)

function(name='glWeightPointerOESBounds', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='type', type='GLenum'),
        Argument(name='stride', type='GLsizei'),
        Argument(name='pointer', type='const GLvoid*'),
        Argument(name='count', type='GLsizei'),
    ],
)

function(name='glWeightbvARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='weights', type='const GLbyte*', wrap_params='size, weights'),
    ],
)

function(name='glWeightdvARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='weights', type='const GLdouble*', wrap_params='size, weights'),
    ],
)

function(name='glWeightfvARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='weights', type='const GLfloat*', wrap_params='size, weights'),
    ],
)

function(name='glWeightivARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='weights', type='const GLint*', wrap_params='size, weights'),
    ],
)

function(name='glWeightsvARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='weights', type='const GLshort*', wrap_params='size, weights'),
    ],
)

function(name='glWeightubvARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='weights', type='const GLubyte*', wrap_params='size, weights'),
    ],
)

function(name='glWeightuivARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='weights', type='const GLuint*', wrap_params='size, weights'),
    ],
)

function(name='glWeightusvARB', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='size', type='GLint'),
        Argument(name='weights', type='const GLushort*', wrap_params='size, weights'),
    ],
)

function(name='glWindowPos2d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
    ],
)

function(name='glWindowPos2dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2d')

function(name='glWindowPos2dMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2d')

function(name='glWindowPos2dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='2, v'),
    ],
)

function(name='glWindowPos2dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2dv')

function(name='glWindowPos2dvMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2dv')

function(name='glWindowPos2f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
    ],
)

function(name='glWindowPos2fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2f')

function(name='glWindowPos2fMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2f')

function(name='glWindowPos2fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='2, v'),
    ],
)

function(name='glWindowPos2fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2fv')

function(name='glWindowPos2fvMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2fv')

function(name='glWindowPos2i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
    ],
)

function(name='glWindowPos2iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2i')

function(name='glWindowPos2iMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2i')

function(name='glWindowPos2iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='2, v'),
    ],
)

function(name='glWindowPos2ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2iv')

function(name='glWindowPos2ivMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2iv')

function(name='glWindowPos2s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
    ],
)

function(name='glWindowPos2sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2s')

function(name='glWindowPos2sMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2s')

function(name='glWindowPos2sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='2, v'),
    ],
)

function(name='glWindowPos2svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2sv')

function(name='glWindowPos2svMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos2sv')

function(name='glWindowPos3d', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
    ],
)

function(name='glWindowPos3dARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3d')

function(name='glWindowPos3dMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3d')

function(name='glWindowPos3dv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='3, v'),
    ],
)

function(name='glWindowPos3dvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3dv')

function(name='glWindowPos3dvMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3dv')

function(name='glWindowPos3f', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
    ],
)

function(name='glWindowPos3fARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3f')

function(name='glWindowPos3fMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3f')

function(name='glWindowPos3fv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='3, v'),
    ],
)

function(name='glWindowPos3fvARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3fv')

function(name='glWindowPos3fvMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3fv')

function(name='glWindowPos3i', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
    ],
)

function(name='glWindowPos3iARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3i')

function(name='glWindowPos3iMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3i')

function(name='glWindowPos3iv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='3, v'),
    ],
)

function(name='glWindowPos3ivARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3iv')

function(name='glWindowPos3ivMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3iv')

function(name='glWindowPos3s', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
    ],
)

function(name='glWindowPos3sARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3s')

function(name='glWindowPos3sMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3s')

function(name='glWindowPos3sv', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='3, v'),
    ],
)

function(name='glWindowPos3svARB', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3sv')

function(name='glWindowPos3svMESA', enabled=True, function_type=FuncType.PARAM, inherit_from='glWindowPos3sv')

function(name='glWindowPos4dMESA', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLdouble'),
        Argument(name='y', type='GLdouble'),
        Argument(name='z', type='GLdouble'),
        Argument(name='w', type='GLdouble'),
    ],
)

function(name='glWindowPos4dvMESA', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLdouble*', wrap_params='4, v'),
    ],
)

function(name='glWindowPos4fMESA', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLfloat'),
        Argument(name='y', type='GLfloat'),
        Argument(name='z', type='GLfloat'),
        Argument(name='w', type='GLfloat'),
    ],
)

function(name='glWindowPos4fvMESA', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLfloat*', wrap_params='4, v'),
    ],
)

function(name='glWindowPos4iMESA', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLint'),
        Argument(name='y', type='GLint'),
        Argument(name='z', type='GLint'),
        Argument(name='w', type='GLint'),
    ],
)

function(name='glWindowPos4ivMESA', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLint*', wrap_params='4, v'),
    ],
)

function(name='glWindowPos4sMESA', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='x', type='GLshort'),
        Argument(name='y', type='GLshort'),
        Argument(name='z', type='GLshort'),
        Argument(name='w', type='GLshort'),
    ],
)

function(name='glWindowPos4svMESA', enabled=True, function_type=FuncType.PARAM,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='v', type='const GLshort*', wrap_params='4, v'),
    ],
)

function(name='glWindowRectanglesEXT', enabled=False, function_type=FuncType.NONE,
    return_value=ReturnValue(type='void'),
    args=[
        Argument(name='mode', type='GLenum'),
        Argument(name='count', type='GLsizei'),
        Argument(name='box', type='const GLint*'),
    ],
)

function(name='glWriteMaskEXT', enabled=True, function_type=FuncType.PARAM,
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
