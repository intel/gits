#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# This module defines the intermediate representation (IR) schema for the
# OpenGL/WGL/GLX/EGL code generator: plain dataclasses describing enums and
# functions, with no XML-parsing or generation logic attached.

from dataclasses import dataclass, field
from enum import Flag, IntFlag

try:
    from enum import StrEnum
except ImportError:
    # StrEnum was introduced in Python 3.11, but we must support 3.10.
    from enum import Enum
    class StrEnum(str, Enum):
        def __str__(self) -> str:
            return self.value


class Api(StrEnum):
    """Plain GL or a native/windowing API. Has a corresponding XML registry file.

    Not to be confused with api="gl"/"gles2" annotations on enums.
    """
    GL  = 'GL'
    WGL = 'WGL'
    EGL = 'EGL'
    GLX = 'GLX'


####### Enum classes #######


class EnumType(StrEnum):
    """Type of an EnumValue. Strings are C literal suffixes, as given in the XML data."""

    INT = ''
    UINT = 'u'
    UINT64 = 'ull'

    def to_type_name(self) -> str:
        """Return OpenGL type name corresponding to the enum value."""
        match self:
            case EnumType.INT:
                return 'GLint'
            case EnumType.UINT:
                return 'GLuint'
            case EnumType.UINT64:
                return 'GLuint64'
            case _:
                raise ValueError(f"Unknown EnumType: {self}")


@dataclass(frozen=True, kw_only=True)
class EnumValue:
    """Single numeric value used as an enum in OpenGL."""

    name: str
    value: str
    type: EnumType = EnumType.INT  # Types other than int appear only thrice in gl.xml.
    comment: str | None = None  # Comments can contain apostrophes, so we enclose them in double quotes.
    groups: tuple[str, ...] = ()  # Can be different from EnumGroup's name, see group="PathRenderingTokenNV" for example.
    alias: str | None = None  # In gl.xml, all aliases match some other enum's name, but they are never mutual.
    api: str | None = None  # Only used twice in gl.xml, for GL_ACTIVE_PROGRAM_EXT: once 'gles2', once 'gl'.
                            # Don't confuse it with the Api enum, which is for GL/WGL/EGL/GLX.


####### Function classes #######


class CmdType(IntFlag):
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
class Parameter:
    """Argument/parameter of a Command."""

    name: str
    type: str
    group: str | None = None
    class_: str | None = None
    kinds: tuple[str, ...] = ()
    len: str | None = None


@dataclass(kw_only=True)
class ReturnValue(Parameter):
    """Return value of a Command."""

    name: str = field(init=False, default='return_value')
    type: str
    group: str | None = None
    class_: str | None = None
    kinds: tuple[str, ...] = ()
    len: str | None = field(init=False, default=None)


@dataclass(kw_only=True)
class Command:
    """API call (or an internal GITS action)."""

    name: str
    api: Api
    enabled: bool
    command_type: CmdType
    comment: str | None = None
    alias: str | None = None
    return_value: ReturnValue
    params: list[Parameter]
