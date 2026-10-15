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


def get_base_type_from_full_type(type_text: str) -> str:
    """Derive a base type from the limited declaration forms used by registries.

    This is intentionally not a general C declarator parser. It assumes that
    the declaration has a base-type portion followed by zero or more pointer
    stars, and that every star belongs to a pointer declarator. The helper has
    only been tested with the current GL/WGL/EGL/GLX registry data.
    """
    normalized = ' '.join(type_text.split())
    base_tokens = normalized.split('*', maxsplit=1)[0].split()
    return ' '.join(token for token in base_tokens if token != 'const')


# Fixed-size value types (across all four registries: GL, WGL, GLX, EGL) that are safe to
# encode/decode as raw bytes, as opposed to pointer-valued or function-pointer-valued
# typedefs - e.g. GLsync, HDC, GLXContext, EGLDisplay, GLDEBUGPROC - that have zero '*' in
# their own declared type text because the pointer (or, for X11 handles, platform ambiguity)
# is baked into the typedef itself (pointer_depth alone cannot tell them apart from a real
# scalar). Every entry below was checked against its registry's own <types> block (or, for
# plain Win32/X11 types absent from Khronos registries, the well-known system typedef) to
# confirm it resolves to a fixed-size integer/float, never a pointer:
#   - GL*: OpenGL-Registry gl.xml.
#   - Win32 (WGL): BOOL/INT/UINT/DWORD/FLOAT/VOID etc. are standard fixed-size Win32
#     typedefs (int/unsigned int/unsigned long/float/void); the handle types in wgl.xml
#     (HDC, HGLRC, HANDLE, PROC, the DECLARE_HANDLE()-based HPBUFFERARB/HGPUNV/etc.) are
#     deliberately excluded here - see glTypes.h, which declares each of those as a pointer
#     type. HDC/HGLRC specifically get a narrower, separate allowance below
#     (_OPAQUE_HANDLE_BASE_TYPES) instead of being folded into this scalar set.
#   - X11 (GLX): 'int'/'unsigned int'/Bool (typedef int) are plain C types; GLXContextID and
#     GLXDrawable are `typedef XID ...` where XID is `unsigned long` (an X11 resource number,
#     not a pointer), same for Font. GLXContext (`typedef struct __GLXcontextRec *`) is
#     deliberately excluded.
#   - EGL (egl.xml <types>): EGLBoolean/EGLenum are `unsigned int`; EGLint is
#     `khronos_int32_t`; EGLAttrib(KHR) is `intptr_t`; EGLTime(KHR|NV)/EGLuint64(KHR|NV)/
#     EGLnsecsANDROID are fixed-width 64-bit integer typedefs; EGLNativeFileDescriptorKHR is
#     `int`. EGLDisplay/EGLContext/EGLSurface/EGLConfig/EGLImage(KHR)/EGLSync(KHR|NV)/
#     EGLStreamKHR/EGLObjectKHR/EGLLabelKHR/EGLOutputLayerEXT/EGLOutputPortEXT/EGLClientBuffer
#     (all `typedef void *`), EGLNativeDisplayType/EGLNativePixmapType (platform-dependent,
#     often a pointer), and the EGL*ANDROID/EGLDEBUGPROCKHR function-pointer typedefs are
#     deliberately excluded.
# 'void' is included only because it is a legal return type, never a parameter type.
_SCALAR_BASE_TYPES: frozenset[str] = frozenset({
    # GL
    'void',
    'GLenum', 'GLboolean', 'GLbitfield',
    'GLbyte', 'GLubyte', 'GLshort', 'GLushort', 'GLint', 'GLuint',
    'GLsizei', 'GLclampx', 'GLfixed',
    'GLfloat', 'GLclampf', 'GLdouble', 'GLclampd', 'GLhalf', 'GLhalfARB', 'GLhalfNV',
    'GLchar', 'GLcharARB',
    'GLintptr', 'GLintptrARB', 'GLsizeiptr', 'GLsizeiptrARB',
    'GLint64', 'GLint64EXT', 'GLuint64', 'GLuint64EXT',
    'GLvdpauSurfaceNV',
    # WGL (Win32)
    'BOOL', 'INT', 'INT32', 'INT64', 'UINT', 'DWORD', 'FLOAT', 'VOID',
    # GLX (X11)
    'int', 'unsigned int', 'Bool', 'GLXContextID', 'GLXDrawable', 'Font',
    # EGL
    'EGLBoolean', 'EGLenum', 'EGLint', 'EGLAttrib', 'EGLAttribKHR',
    'EGLTime', 'EGLTimeKHR', 'EGLTimeNV', 'EGLuint64NV', 'EGLuint64KHR',
    'EGLnsecsANDROID', 'EGLNativeFileDescriptorKHR',
})

# Native OS handles that are safe to capture as a raw opaque value, deliberately kept separate
# from _SCALAR_BASE_TYPES above. Unlike a GL scalar (a GLenum/GLuint/etc. whose bit pattern is
# meaningful on its own), this is a pointer to an OS-owned object that GITS never dereferences
# and, crucially, never creates through an intercepted call chain it fully controls end to end:
# HDC is obtained via GetDC/BeginPaint (User32/GDI32), which this interceptor does not hook (see
# generator_interceptor.py: only GL + WGL commands from wgl.xml are intercepted). Capturing it
# verbatim (as a plain fixed-size field, same as any scalar) matches the existing precedent for
# the exact same problem in Vulkan/common/layer_interface/commandsCustom.h's
# CreateWindowMetaCommand/UpdateWindowMetaCommand, which capture a native HWND as a plain
# Argument<uint64_t> - no key, no handle map - because Vulkan doesn't own that handle's lifecycle
# either. A captured value here is replay-inert until a context/window-tracking service exists to
# turn it into a live handle (see modern-opengl-backend-architecture.md's "Model context topology
# before broad object state"). Contrast with _MAPPED_HANDLE_BASE_TYPES below, for the one native
# handle type (HGLRC) GITS *does* own the full lifecycle of.
_OPAQUE_HANDLE_BASE_TYPES: frozenset[str] = frozenset({
    'HDC',
})

# Native OS/GL handles GITS owns the full lifecycle of: HGLRC is created by an intercepted call
# (wglCreateContext/wglCreateContextAttribsARB) and destroyed by one (wglDeleteContext); GLsync
# is created by glFenceSync and destroyed by glDeleteSync. Unlike HDC above, there is no reason
# to treat either as replay-inert: both get real GITSKey-based mapping (see arguments.h's
# HandleArgument<T>, ContextHandleMapService for HGLRC, and SyncHandleMapService for GLsync),
# the same treatment Vulkan gives every handle type it creates/destroys - see
# modern-opengl-backend-architecture.md's "Real key-based mapping for handles GITS creates"
# decision, which narrows (for exactly these two types) the earlier "Capture native OS handles
# as opaque scalar values, defer remapping" decision.
#
# GLsync is structurally the same shape as HGLRC (both are opaque pointer typedefs GITS fully
# controls the creation/destruction of), but is NOT a GLuint-named object, despite gl.xml
# tagging it with its own `class="sync"` annotation (the same mechanism used for texture/
# buffer/program/... names) - see object_name_lifecycle.py's _CLASS_TO_KIND, which deliberately
# maps 'sync' to None (no GLObjectKind), and intermediates_creator.py's
# _requires_object_name_override, which now excludes every is_mapped_handle field from the
# object-name gate for exactly this reason, regardless of what `class` gl.xml also attaches to
# it.
#
# Every mapped-handle type shares ONE generic map service, HandleMapService<HandleT> (see
# common/services/handleMapService.h) - ContextHandleMapService/SyncHandleMapService are just
# its two type aliases today, not two separate classes. This used to be two hand-copied,
# same-shaped classes (one per type), on the theory that "Keep abstractions backend-local until
# reuse is demonstrated" favored two independently-reviewable classes over a shared template
# with no second user yet; GLsync becoming that second user is exactly the "reuse demonstrated"
# trigger that decision record was waiting for, so the two were merged into one template - see
# handle_lifecycle.py's own MappedHandleLifecycle for the matching Python-side generalization.
_MAPPED_HANDLE_BASE_TYPES: frozenset[str] = frozenset({
    'HGLRC',
    'GLsync',
})

# Fixed-size POD structs (not scalars - pointer_depth == 0 never applies to a whole-struct
# by-value parameter in this backend's command set, so these deliberately do not join
# _SCALAR_BASE_TYPES/is_scalar) that are nonetheless safe to memcpy whole as a single array
# element via ArrayArgument<T> (see arguments.h/argumentCoders.h, both already fully generic
# over T - no template specialization needed for a struct vs. a scalar). PIXELFORMATDESCRIPTOR
# is the one entry needed so far, for wglSetPixelFormat's `ppfd` (see
# docs/development/opengl2-player-skeleton-plan.md's Decision 9); see is_array_element_eligible
# below for how an entry here joins _SCALAR_BASE_TYPES at pointer_depth == 1.
_FIXED_SIZE_STRUCT_BASE_TYPES: frozenset[str] = frozenset({
    'PIXELFORMATDESCRIPTOR',
})


@dataclass(frozen=True, kw_only=True)
class TypeFacts:
    """Derived facts from a C type declaration."""

    base_type: str
    pointer_depth: int

    @classmethod
    def from_type(cls, type_text: str) -> 'TypeFacts':
        normalized = ' '.join(type_text.split())
        pointer_depth = normalized.count('*')

        return cls(
            base_type=get_base_type_from_full_type(type_text),
            pointer_depth=pointer_depth,
        )

    @property
    def is_scalar(self) -> bool:
        """Whether this is a fixed-size GL value type with no pointer semantics at all."""
        return self.pointer_depth == 0 and self.base_type in _SCALAR_BASE_TYPES

    @property
    def is_opaque_handle(self) -> bool:
        """Whether this is a native OS handle safe to capture as a raw, unremapped value.

        See _OPAQUE_HANDLE_BASE_TYPES for the rationale and its explicit non-goals (no
        key-based translation, no replay-time reconstruction yet). Mutually exclusive with
        is_mapped_handle - see that property for the handle types GITS does remap.
        """
        return self.pointer_depth == 0 and self.base_type in _OPAQUE_HANDLE_BASE_TYPES

    @property
    def is_mapped_handle(self) -> bool:
        """Whether this is a native OS handle GITS owns the lifecycle of and gives a real,
        GITSKey-based mapping (see _MAPPED_HANDLE_BASE_TYPES and arguments.h's
        HandleArgument<T>), as opposed to a raw, unremapped opaque value (is_opaque_handle).
        """
        return self.pointer_depth == 0 and self.base_type in _MAPPED_HANDLE_BASE_TYPES

    @property
    def is_array_element_eligible(self) -> bool:
        """Whether this is a single-indirection pointer to a fixed-size element type this
        increment's array/pointer coder system (see arguments.h's ArrayArgument<T>) can memcpy
        directly - i.e. pointer_depth == 1 and the base type is one of the same fixed-size types
        is_scalar already trusts at pointer_depth == 0 (_SCALAR_BASE_TYPES already includes
        'void', so 'const void *' data blobs - glBufferData et al. - are covered here without a
        separate byte-blob type; see array_element_cpp_type() in generator_helpers.py for how
        'void' becomes a real C++ template argument).

        This is a necessary, not sufficient, condition for capturing a pointer parameter: it says
        the *element type* is safe to memcpy, not that this specific parameter's direction/length
        are known. See pointer_metadata.py for the second half of that classification - a
        hand-maintained table, not a derived fact, per "Classify payloads from evidence."
        Mutually exclusive with is_scalar (pointer_depth differs) and with is_opaque_handle/
        is_mapped_handle (those are pointer_depth == 0 by definition, covering the handle
        *itself*, not a pointer *to* one - HDC*/HGLRC* out-parameters are not covered here and
        stay unsupported, same as before this change).

        Also true for a single-indirection pointer to a _FIXED_SIZE_STRUCT_BASE_TYPES entry
        (currently just PIXELFORMATDESCRIPTOR) - same memcpy-safety argument, just for a POD
        struct instead of a scalar; see that set's own comment.
        """
        return self.pointer_depth == 1 and (
            self.base_type in _SCALAR_BASE_TYPES or self.base_type in _FIXED_SIZE_STRUCT_BASE_TYPES)

    @property
    def is_array_of_arrays_eligible(self) -> bool:
        """Whether this is a doubly-indirect pointer (`const T* const*`) to the same fixed-size
        element types is_array_element_eligible trusts at pointer_depth == 1 - i.e.
        pointer_depth == 2 and the base type is in _SCALAR_BASE_TYPES or
        _FIXED_SIZE_STRUCT_BASE_TYPES. gl.xml's only occurrence today is glShaderSource's
        `const GLchar* const* string` (see array_of_arrays_metadata.py's ArrayOfArraysOverride).

        Mutually exclusive with is_array_element_eligible (pointer_depth differs). Like that
        property, this is a necessary, not sufficient, condition: it says only that the
        *element* type is memcpy-safe, not that this specific parameter's shape (the outer
        count + per-element length sibling parameters) is known - see
        array_of_arrays_metadata.py for that second half of the classification.
        """
        return self.pointer_depth == 2 and (
            self.base_type in _SCALAR_BASE_TYPES or self.base_type in _FIXED_SIZE_STRUCT_BASE_TYPES)


@dataclass(kw_only=True)
class Parameter:
    """Argument/parameter of a Command."""

    name: str
    type: str
    group: str | None = None
    class_: str | None = None
    kinds: tuple[str, ...] = ()
    len: str | None = None
    type_facts: TypeFacts = field(init=False)

    def __post_init__(self) -> None:
        self.type_facts = TypeFacts.from_type(self.type)


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
