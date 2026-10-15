#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Small, hand-maintained table classifying pointer-shaped command parameters - precedent:
# handle_lifecycle.py's HANDLE_CREATING_COMMANDS/HANDLE_PARAM_TO_ERASE_ON_SUCCESS. Pointer
# direction (in/out) and length (how many elements) cannot be derived from gl.xml/wgl.xml alone
# (see modern-opengl-backend-architecture.md, "Classify payloads from evidence": "do not infer
# output solely from a non-const pointer"; GL's registry has no in/out annotation at all), so
# this is a per-(command, param) manual claim, corroborated against Parameter.len where present
# but not inferred from it. See intermediates_creator.py's _is_param_capturable/
# _validate_pointer_override for how this table gates the `enabled` flag.

from dataclasses import dataclass

try:
    from enum import StrEnum
except ImportError:
    from enum import Enum
    class StrEnum(str, Enum):
        def __str__(self) -> str:
            return self.value


class PointerDirection(StrEnum):
    """Which side of the driver call owns the bytes at capture time.

    INOUT (a single pointer serving both directions, e.g. glGetActiveUniform-style APIs) is
    deliberately absent - no command in this increment's candidate set needs it. Add it only
    once a real command does.
    """
    IN = 'in'
    OUT = 'out'


class LengthSourceKind(StrEnum):
    """How many elements this pointer refers to, and how that count is discovered."""
    SIBLING_PARAM = 'sibling_param'    # length_source_value = another param's name (by-value int)
    FIXED_COUNT = 'fixed_count'        # length_source_value = a literal, e.g. '1' (unused by the
                                        # 8 commands below; kept for the next easy win - single-
                                        # value `iv` getters like glGetShaderiv always return
                                        # exactly one GLint).
    NULL_TERMINATED = 'null_terminated'  # scan forward for a T{} sentinel; length_source_value
                                          # is unused (None).
    KEY_VALUE_LIST = 'key_value_list'  # scan forward in (key, value) PAIRS, terminating only
        # when a pair's KEY element is zero - length_source_value is unused (None). Distinct
        # from NULL_TERMINATED: a WGL-style attribute list (wglCreateContextAttribsARB's
        # `attribList`, and the same {key, value, ..., 0} shape used by wglChoosePixelFormatARB/
        # wglCreatePbufferARB/wglCreateAssociatedContextAttribsAMD's own attribute-list params,
        # none of which are enabled today - see this module's own POINTER_PARAM_OVERRIDES entry
        # comment) is terminated ONLY by a zero KEY; zero is a perfectly legal VALUE for some
        # keys (e.g. WGL_CONTEXT_FLAGS_ARB = 0, WGL_CONTEXT_LAYER_PLANE_ARB = 0). Scanning
        # element-by-element like NULL_TERMINATED does would stop at that legal zero VALUE
        # instead of the real terminating zero KEY two elements later, silently truncating the
        # list - see arguments.h's KeyValueListTag-based ArrayArgument<T> constructor for the
        # actual pairs-aware scan this drives.
    OPAQUE_BIT_PATTERN = 'opaque_bit_pattern'  # not an array at all - capture the pointer's own
        # numeric value (as a uintptr_t) and never dereference it. See
        # docs/development/opengl2-rendering-pipeline-pointer-support-plan.md's offset-vs-pointer
        # decision: glVertexAttribPointer's `pointer` / glDrawElements' `indices` mean either a
        # real client-memory pointer or a GL_ARRAY_BUFFER/GL_ELEMENT_ARRAY_BUFFER-relative byte
        # offset, depending on live GL binding state gl.xml cannot express and this backend does
        # not track - this increment captures the bit pattern verbatim (correct for the
        # buffer-offset form every real 3.2+ core-profile caller uses; a genuine client-memory
        # pointer round-trips its *value*, not its *pointee*, which is a documented, deliberate
        # non-goal - see that decision's Consequences). direction is always IN (there is exactly
        # one already-known value; nothing is written back) and length_source_value is unused
        # (None) - there is no length, just one opaque value.
    PIXEL_TRANSFER_SIZE = 'pixel_transfer_size'  # length_source_value is unused (None); instead
        # four extra PointerParamOverride fields (pixel_format_param/pixel_type_param/
        # pixel_width_param/pixel_height_param) name the sibling params gl.xml's
        # COMPSIZE(format,type,width,height) pseudo-length expression actually depends on (see
        # docs/development/opengl2-player-skeleton-plan.md's Decision 10). No single sibling
        # param holds this length by itself - unlike every SIBLING_PARAM entry above, which reads
        # one param's value directly - so the byte count is computed at capture time by a real
        # runtime helper (gits::opengl2::ComputePixelTransferByteSize(),
        # OpenGL2/common/layer_interface/pixelTransferSize.h) from all four values together.
    CLEAR_BUFFER_VALUE_COUNT = 'clear_buffer_value_count'  # length_source_value is unused
        # (None); instead the extra `clear_buffer_param` field names the sibling `buffer` enum
        # param glClearBufferfv/glClearBufferiv/glClearBufferuiv's len="COMPSIZE(buffer)" value
        # count actually depends on (4 floats/ints for GL_COLOR, 1 for GL_DEPTH/GL_STENCIL - see
        # docs/development/opengl2-player-skeleton-plan.md's Decision 10). Like
        # PIXEL_TRANSFER_SIZE, the sibling's *value* (not just its presence) determines the
        # count, so a real runtime helper (gits::opengl2::ComputeClearBufferValueCount(),
        # OpenGL2/common/layer_interface/pixelTransferSize.h) computes it at capture time
        # instead of a SIBLING_PARAM direct by-value read.


@dataclass(frozen=True, kw_only=True)
class PointerParamOverride:
    direction: PointerDirection
    length_source_kind: LengthSourceKind
    length_source_value: str | None = None  # sibling name, fixed-count literal, or None
    length_multiplier: int = 1  # element count = length_source_value's value * length_multiplier
        # - e.g. glUniform4fv's `count` sibling gives *vectors*, not *floats*; gl.xml's own
        # len="count*4" annotation gives the multiplier directly. Unused (stays 1) by every
        # SIBLING_PARAM entry that already counts elements 1:1 with its sibling's value.
    # Only meaningful when length_source_kind is PIXEL_TRANSFER_SIZE - the four sibling param
    # names ComputePixelTransferByteSize() needs (see that LengthSourceKind member's own
    # comment). None for every other kind.
    pixel_format_param: str | None = None
    pixel_type_param: str | None = None
    pixel_width_param: str | None = None
    pixel_height_param: str | None = None

    # Only meaningful when length_source_kind is CLEAR_BUFFER_VALUE_COUNT - the sibling
    # `buffer` enum param name ComputeClearBufferValueCount() needs. None for every other kind.
    clear_buffer_param: str | None = None


# (command_name, param_name) -> classification. Every entry here is a manual claim about GL/WGL
# semantics gl.xml/wgl.xml cannot fully express (see modern-opengl-backend-architecture.md,
# "Classify payloads from evidence"). intermediates_creator.py cross-checks SIBLING_PARAM entries
# against the command's actual parameter list and raises at generation time on a mismatch (a
# broken table entry is a bug, not a "stay disabled" situation) - see
# _validate_pointer_override(). Parameters with no entry here simply stay unsupported; absence is
# always safe, presence is always validated.
#
# Every len=/absence-of-len claim below was verified directly against the live
# third_party/OpenGL-Registry/xml/{gl,wgl}.xml checked out in this workspace (see the
# implementation report for citations) - glGenTextures/glDeleteTextures/glGenBuffers's array
# params carry len="n", glBufferData/glBufferSubData's `data` carries len="size", and
# glGetUniformLocation/glBindAttribLocation's `name` and wglCreateContextAttribsARB's
# `attribList` all carry no len attribute at all, consistent with NULL_TERMINATED/
# KEY_VALUE_LIST below (gl.xml/wgl.xml make no distinction between the two - both are simply
# "no len=" - so which one applies is itself a manual claim about the parameter's own
# element-vs-pair termination semantics, not something the registry states directly).
POINTER_PARAM_OVERRIDES: dict[tuple[str, str], PointerParamOverride] = {
    ('glGenTextures', 'textures'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    ('glDeleteTextures', 'textures'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    ('glGenBuffers', 'buffers'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    ('glBufferData', 'data'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='size'),
    ('glBufferSubData', 'data'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='size'),
    ('glGetUniformLocation', 'name'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.NULL_TERMINATED),
    ('glBindAttribLocation', 'name'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.NULL_TERMINATED),
    # A {key, value, key, value, ..., 0} attribute list, terminated only by a zero KEY - see
    # LengthSourceKind.KEY_VALUE_LIST's own comment above for why this must not use plain
    # NULL_TERMINATED. wglChoosePixelFormatARB/wglChoosePixelFormatEXT's own piAttribIList/
    # pfAttribFList and wglCreatePbufferARB/EXT's piAttribList and
    # wglCreateAssociatedContextAttribsAMD's attribList share this exact same shape but stay
    # unsupported (no override entry) - none of the 8 currently-enabled commands' own callers
    # need them, and adding one speculatively without a tested caller would be exactly the kind
    # of unverified table entry this file's own header comment warns against.
    ('wglCreateContextAttribsARB', 'attribList'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.KEY_VALUE_LIST),
    # A single PIXELFORMATDESCRIPTOR struct, not an array - wgl.xml's `ppfd` carries no len=
    # attribute at all (see docs/development/opengl2-player-skeleton-plan.md's Decision 9).
    #
    # Dual-registration: this is the interceptor-exported name (generator_interceptor.py's
    # _WGL_RENAMED_FROM_UNPREFIXED renames wgl.xml's raw `SetPixelFormat` to `wglSetPixelFormat`).
    # intermediates_creator.py's enabled-gate runs on the *raw* per-registry name before that
    # rename, while every Mako template's pointer_override_for() call runs on the *renamed*
    # command list generator.py builds everything else from - so this same override needs both
    # keys until a second renamed-and-overridden command justifies fixing this at the framework
    # level instead (see Decision 9's own "Revisit when").
    ('SetPixelFormat', 'ppfd'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('wglSetPixelFormat', 'ppfd'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),

    # Buffer-relative offsets disguised as pointers (see
    # docs/development/opengl2-rendering-pipeline-pointer-support-plan.md's offset-vs-pointer
    # decision): captured as a bit pattern, never dereferenced - see
    # LengthSourceKind.OPAQUE_BIT_PATTERN's own comment above.
    ('glVertexAttribPointer', 'pointer'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN),
    ('glDrawElements', 'indices'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN),
    ('glDrawElementsInstanced', 'indices'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN),
    ('glDrawElementsInstancedBaseVertex', 'indices'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN),

    # Vector/matrix uniform setters: `count` counts *vectors*/*matrices*, not raw elements -
    # gl.xml's own len="count*N" annotation gives each one's multiplier directly (see
    # docs/development/opengl2-rendering-pipeline-pointer-support-plan.md's Decision 3).
    ('glUniform1fv', 'value'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='count'),
    ('glUniform2fv', 'value'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.SIBLING_PARAM,
        length_source_value='count', length_multiplier=2),
    ('glUniform3fv', 'value'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.SIBLING_PARAM,
        length_source_value='count', length_multiplier=3),
    ('glUniform4fv', 'value'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.SIBLING_PARAM,
        length_source_value='count', length_multiplier=4),
    ('glUniform1iv', 'value'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='count'),
    ('glUniform2iv', 'value'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.SIBLING_PARAM,
        length_source_value='count', length_multiplier=2),
    ('glUniform3iv', 'value'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.SIBLING_PARAM,
        length_source_value='count', length_multiplier=3),
    ('glUniform4iv', 'value'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.SIBLING_PARAM,
        length_source_value='count', length_multiplier=4),
    ('glUniform1uiv', 'value'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='count'),
    ('glUniform2uiv', 'value'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.SIBLING_PARAM,
        length_source_value='count', length_multiplier=2),
    ('glUniform3uiv', 'value'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.SIBLING_PARAM,
        length_source_value='count', length_multiplier=3),
    ('glUniform4uiv', 'value'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.SIBLING_PARAM,
        length_source_value='count', length_multiplier=4),
    ('glUniformMatrix2fv', 'value'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.SIBLING_PARAM,
        length_source_value='count', length_multiplier=4),
    ('glUniformMatrix3fv', 'value'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.SIBLING_PARAM,
        length_source_value='count', length_multiplier=9),
    ('glUniformMatrix4fv', 'value'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.SIBLING_PARAM,
        length_source_value='count', length_multiplier=16),

    # VAO/FBO array-output creation and array-input destruction - same SIBLING_PARAM('n') shape
    # as glGenTextures/glDeleteTextures above (see
    # docs/development/opengl2-player-skeleton-plan.md's Decision 10).
    ('glGenVertexArrays', 'arrays'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    ('glDeleteVertexArrays', 'arrays'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    ('glGenFramebuffers', 'framebuffers'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    ('glDeleteFramebuffers', 'framebuffers'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),

    # glTexImage2D's `pixels`: byte size depends on format/type/width/height together (gl.xml's
    # own COMPSIZE(format,type,width,height)) - see LengthSourceKind.PIXEL_TRANSFER_SIZE's own
    # comment above and OpenGL2/common/layer_interface/pixelTransferSize.h.
    ('glTexImage2D', 'pixels'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.PIXEL_TRANSFER_SIZE,
        pixel_format_param='format', pixel_type_param='type',
        pixel_width_param='width', pixel_height_param='height'),

    # glClearBufferfv/iv/uiv's `value`: element count is 4 for GL_COLOR, 1 for
    # GL_DEPTH/GL_STENCIL, decoded from the sibling `buffer` enum - see
    # LengthSourceKind.CLEAR_BUFFER_VALUE_COUNT's own comment above.
    ('glClearBufferfv', 'value'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.CLEAR_BUFFER_VALUE_COUNT,
        clear_buffer_param='buffer'),
    ('glClearBufferiv', 'value'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.CLEAR_BUFFER_VALUE_COUNT,
        clear_buffer_param='buffer'),
    ('glClearBufferuiv', 'value'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.CLEAR_BUFFER_VALUE_COUNT,
        clear_buffer_param='buffer'),

    # Query-and-fixed-size-getter round: single-GLint getters and null-terminated string
    # lookups that were simply missing a table entry, not structurally unsupported - see
    # modern-opengl-backend-architecture.md's "Reduce replay warning noise" decision.
    #
    # glGetShaderiv/glGetProgramiv/glGetFramebufferAttachmentParameteriv's `params` all carry
    # len="COMPSIZE(pname)" in gl.xml, but unlike glGetIntegerv's genuinely pname-dependent
    # count (1/2/4/16 elements), the GL/GLES specs guarantee these three always write exactly
    # one GLint regardless of pname - COMPSIZE(pname) here is a registry-wide annotation
    # convention, not evidence of a variable count for *these* three commands specifically.
    ('glGetShaderiv', 'params'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('glGetProgramiv', 'params'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('glGetFramebufferAttachmentParameteriv', 'params'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),

    # Null-terminated string lookups - identical shape to glGetUniformLocation/
    # glBindAttribLocation's own `name` above, just on two more commands. gl.xml annotates
    # both as len="COMPSIZE(name)"/len="COMPSIZE()" (a self-referential placeholder meaning
    # "computed from the string itself"), not a real sibling length - still a plain
    # null-terminated GLchar* in both cases.
    ('glBindFragDataLocation', 'name'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.NULL_TERMINATED),
    ('glGetUniformBlockIndex', 'uniformBlockName'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.NULL_TERMINATED),

    # glGenQueries/glDeleteQueries: same SIBLING_PARAM('n') array-output-creation/array-input-
    # destruction shape as glGenTextures/glDeleteTextures and glGenVertexArrays/
    # glGenFramebuffers above.
    ('glGenQueries', 'ids'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    ('glDeleteQueries', 'ids'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),

    # glGenRenderbuffers/glDeleteRenderbuffers, glGenSamplers/glDeleteSamplers,
    # glGenProgramPipelines/glDeleteProgramPipelines: same SIBLING_PARAM array-output-creation/
    # array-input-destruction shape as glGenQueries/glGenVertexArrays/glGenFramebuffers above -
    # added (see object_name_lifecycle.py's matching
    # OBJECT_NAME_PARAM_OVERRIDES entries) to close a real, previously-silent replay-corruption
    # gap: all three Bind commands (glBindRenderbuffer/glBindSampler/glBindProgramPipeline)
    # were already `enabled=True` (scalar-only) with no lifecycle tracking of any kind for
    # their object namespace, so their GLuint name was passed straight through to the replay
    # driver unresolved. glGenSamplers/glDeleteSamplers's sibling count param is named `count`,
    # not `n` - the other two follow every other Gen/Delete pair's `n` convention.
    ('glGenRenderbuffers', 'renderbuffers'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    ('glDeleteRenderbuffers', 'renderbuffers'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    ('glGenSamplers', 'samplers'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='count'),
    ('glDeleteSamplers', 'samplers'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='count'),
    ('glGenProgramPipelines', 'pipelines'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    ('glDeleteProgramPipelines', 'pipelines'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),

    # glReadPixels' `pixels`: an OUT-direction pixel-transfer buffer whose byte size depends on
    # format/type/width/height together, same COMPSIZE(format,type,width,height) shape
    # glTexImage2D's `pixels` already uses (see LengthSourceKind.PIXEL_TRANSFER_SIZE's own
    # comment above) - just captured after the driver call instead of copied before it, since
    # the driver (not the app) writes these bytes. This is the first OUT-direction user of
    # PIXEL_TRANSFER_SIZE; recorderWrappersAuto.cpp.mako's post-call Capture() branch was
    # extended to compute the byte count the same way commandsAuto.h.mako's IN-direction
    # constructor branch already does, rather than assuming a single sibling value.
    ('glReadPixels', 'pixels'): PointerParamOverride(
        direction=PointerDirection.OUT, length_source_kind=LengthSourceKind.PIXEL_TRANSFER_SIZE,
        pixel_format_param='format', pixel_type_param='type',
        pixel_width_param='width', pixel_height_param='height'),

    # wglDescribePixelFormat's `ppfd`: a single OUT PIXELFORMATDESCRIPTOR struct - the OUT
    # counterpart of wglSetPixelFormat/SetPixelFormat's own IN `ppfd` above, same FIXED_COUNT
    # shape. Needs both the raw registry name (`DescribePixelFormat`) and the interceptor-
    # exported renamed one (`wglDescribePixelFormat`) - see SetPixelFormat/wglSetPixelFormat's
    # own comment above for why both keys are required.
    ('DescribePixelFormat', 'ppfd'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('wglDescribePixelFormat', 'ppfd'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),

    # wglGetPixelFormatAttribivARB's `piAttributes`/`piValues`: two independent arrays sharing
    # the same sibling length (`nAttributes`) - an IN query-key array and an OUT query-result
    # array, both plain SIBLING_PARAM shapes; no new mechanism needed for two array fields to
    # reference the same sibling.
    ('wglGetPixelFormatAttribivARB', 'piAttributes'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='nAttributes'),
    ('wglGetPixelFormatAttribivARB', 'piValues'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='nAttributes'),
}
