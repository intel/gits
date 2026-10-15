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

TEXT_OUTPUT_BUFFERS: dict[tuple[str, str], str] = {
    ('glGetActiveAttrib', 'name'): 'length',
    ('glGetActiveUniform', 'name'): 'length',
    ('glGetTransformFeedbackVarying', 'name'): 'length',
    ('glGetProgramInfoLog', 'infoLog'): 'length',
    ('glGetProgramPipelineInfoLog', 'infoLog'): 'length',
    ('glGetShaderInfoLog', 'infoLog'): 'length',
    ('glGetActiveUniformBlockName', 'uniformBlockName'): 'length',
    ('glGetActiveUniformName', 'uniformName'): 'length',
}

try:
    from enum import StrEnum
except ImportError:
    from enum import Enum
    class StrEnum(str, Enum):
        def __str__(self) -> str:
            return self.value


class PointerDirection(StrEnum):
    """Which side of the driver call owns the bytes at capture time.

    INOUT (a single pointer serving both directions) is deliberately absent - no command in
    this increment's candidate set needs it. glGetActiveUniform-style APIs (bufSize/length/
    size/type/name) looked like a plausible candidate before being audited, but every one of
    their pointer fields turns out to be purely IN (bufSize) or purely OUT (length/size/type/
    name) - see this file's own glGetActiveUniform/glGetActiveAttrib/
    glGetTransformFeedbackVarying entries below. Add INOUT only once a real command genuinely
    needs one pointer serving both directions.
    """
    IN = 'in'
    OUT = 'out'


class LengthSourceKind(StrEnum):
    """How many elements this pointer refers to, and how that count is discovered."""
    SIBLING_PARAM = 'sibling_param'  # length_source_value = another param's name (by-value int)
        # value). Most entries of this kind are hand-authored directly below, but the whole
        # glUniform*v/glProgramUniform*v/glUniformMatrix*v/glProgramUniformMatrix*v family is
        # now auto-derived instead - see uniform_setter_metadata.py's
        # derive_uniform_setter_override(), consulted as a fallback by
        # intermediates_creator.py's _is_param_capturable / generator_helpers.py's
        # pointer_override_for() whenever this table has no explicit entry for a `value` param
        # matching that module's own closed family regexes (a narrower, sibling-family
        # counterpart of PIXEL_TRANSFER_SIZE's own COMPSIZE(format,type,...) auto-derivation
        # below - see that module's header comment for why this one IS safe despite this
        # class's own "direction/category never derivable from gl.xml alone" rule).
    FIXED_COUNT = 'fixed_count'  # length_source_value = a literal, e.g. '1' (unused by the 8
                                 # commands below; kept for the next easy win - single-value `iv`
                                 # getters like glGetShaderiv always return exactly one GLint).
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
    OPAQUE_BIT_PATTERN_ARRAY = 'opaque_bit_pattern_array'  # a doubly-indirect pointer
        # (`const void* const*`) whose own ELEMENTS are opaque bit patterns (never dereferenced),
        # not an array of arrays of real bytes - the multi-draw counterpart of
        # OPAQUE_BIT_PATTERN above. E.g. glMultiDrawElementsBaseVertex's `indices`: gl.xml gives
        # it the exact same `const void* const*` shape as glShaderSource's `string`
        # (TypeFacts.is_array_of_arrays_eligible is true for both - pointer_depth == 2, base
        # type 'void'), but the *meaning* is completely different. glShaderSource's `string[i]`
        # is a real pointer to source-code bytes this backend must copy and later hand back a
        # live pointer to (ArrayOfArrays<T> - see that type's own comment). glMultiDraw*'s
        # `indices[i]` is exactly like glDrawElements' single `indices` (OPAQUE_BIT_PATTERN
        # above) repeated `drawcount` times: each element is itself either a
        # GL_ELEMENT_ARRAY_BUFFER-relative byte offset or a genuine client-memory pointer, and
        # must never be dereferenced - only its bit pattern round-trips. Using ArrayOfArrays<T>
        # here would be actively wrong (it would try to copy each indices[i]'s *pointee* bytes,
        # which for the offset form is not even a valid address). length_source_value names the
        # sibling drawcount-style param, exactly like SIBLING_PARAM - see arguments.h's
        # ArrayArgument<T>, which this reuses directly (each "element" is just a pointer-sized
        # uintptr_t, memcpy-safe like any other scalar) rather than adding a second array-of-
        # opaque-values type.
    PIXEL_TRANSFER_SIZE = 'pixel_transfer_size'  # length_source_value is unused (None); instead
        # five extra PointerParamOverride fields (pixel_format_param/pixel_type_param/
        # pixel_width_param/pixel_height_param/pixel_depth_param) name the sibling params
        # gl.xml's COMPSIZE(format,type,width[,height[,depth]]) pseudo-length expression
        # actually depends on (see docs/development/opengl2-player-skeleton-plan.md's Decision
        # 10). No single sibling param holds this length by itself - unlike every SIBLING_PARAM
        # entry above, which reads one param's value directly - so the byte count is computed
        # at capture time by a real runtime helper (gits::opengl2::
        # ComputePixelTransferByteSize(), OpenGL2/common/layer_interface/pixelTransferSize.h)
        # from these values together. pixel_height_param/pixel_depth_param are optional (None
        # for a 1D transfer's plain `width`-only shape; pixel_depth_param additionally requires
        # pixel_height_param, since no 1D-with-depth shape exists) - Mako call sites substitute
        # a literal `1` for either when absent, matching ComputePixelTransferByteSize()'s own
        # height/depth<=0 short-circuit doing nothing for a dimension that was never real to
        # begin with. Most entries needing this kind are now auto-derived rather than
        # hand-written - see pixel_transfer_metadata.py's derive_pixel_transfer_override(),
        # consulted as a fallback by intermediates_creator.py's _is_param_capturable /
        # generator_helpers.py's pointer_override_for() whenever this table has no explicit
        # entry for a (command, param) pair. A manual entry here still always wins over that
        # fallback (e.g. a future 1D-vs-2D-vs-3D-vs-array-vs-cube variant needing a different
        # call shape).
    CLEAR_BUFFER_VALUE_COUNT = 'clear_buffer_value_count'  # length_source_value is unused
        # (None); instead the extra `clear_buffer_param` field names the sibling `buffer` enum
        # param glClearBufferfv/glClearBufferiv/glClearBufferuiv's len="COMPSIZE(buffer)" value
        # count actually depends on (4 floats/ints for GL_COLOR, 1 for GL_DEPTH/GL_STENCIL - see
        # docs/development/opengl2-player-skeleton-plan.md's Decision 10). Like
        # PIXEL_TRANSFER_SIZE, the sibling's *value* (not just its presence) determines the
        # count, so a real runtime helper (gits::opengl2::ComputeClearBufferValueCount(),
        # OpenGL2/common/layer_interface/pixelTransferSize.h) computes it at capture time
        # instead of a SIBLING_PARAM direct by-value read.
    TEX_SAMPLER_PARAMETER_VALUE_COUNT = 'tex_sampler_parameter_value_count'  # length_source_value
        # is unused (None); instead the extra `pname_param` field names the sibling `pname` enum
        # param whose len="COMPSIZE(pname)" value count actually depends on, and the extra
        # `tex_sampler_family` field (a TexOrSamplerParameterFamily - see this module's own
        # comment on that class below) names which of the two command families is calling: 4
        # for GL_TEXTURE_BORDER_COLOR regardless of family, 4 for GL_TEXTURE_SWIZZLE_RGBA only
        # when `tex_sampler_family` is TEX_PARAMETER (1 - not 4 - when it is
        # SAMPLER_PARAMETER, since GL_TEXTURE_SWIZZLE_RGBA is not a legal pname for any
        # glSamplerParameter*v form at all - verified against gl.xml; see
        # tex_sampler_parameter_metadata.py's own header comment), 1 for every other legal
        # pname. Same "sibling's value, not just its presence,
        # determines the count" shape as CLEAR_BUFFER_VALUE_COUNT above - see gits::opengl2::
        # ComputeTexOrSamplerParameterValueCount()/TexOrSamplerParameterCommand
        # (pixelTransferSize.h). IN direction only today (every enabled user is a setter, and
        # the Mako templates have no OUT-direction branch for this kind) - see
        # _validate_pointer_override's own direction check. Every
        # glTexParameter{,I}{i,ui}v/glTexParameterfv/glSamplerParameter{,I}{i,ui}v/
        # glSamplerParameterfv sibling is now auto-derived rather than hand-entered here - see
        # tex_sampler_parameter_metadata.py's derive_tex_sampler_parameter_override(), consulted
        # as a third fallback by intermediates_creator.py's _is_param_capturable/
        # generator_helpers.py's pointer_override_for() whenever this table has no explicit
        # entry for the (command, param) pair.
    PATCH_PARAMETER_VALUE_COUNT = 'patch_parameter_value_count'  # length_source_value is unused
        # (None); instead the extra `pname_param` field names the sibling `pname` enum param
        # glPatchParameterfv's len="COMPSIZE(pname)" value count actually depends on: 4 for
        # GL_PATCH_DEFAULT_OUTER_LEVEL, 2 for GL_PATCH_DEFAULT_INNER_LEVEL (the only two legal
        # pnames). See gits::opengl2::ComputePatchParameterValueCount() (pixelTransferSize.h).
        # IN direction only today - see _validate_pointer_override's own direction check.
    MATERIAL_PARAMETER_VALUE_COUNT = 'material_parameter_value_count'  # length_source_value is
        # unused (None); instead the extra `pname_param` field names the sibling `pname` enum
        # param glMaterialfv's len="COMPSIZE(pname)" value count actually depends on: 4 for
        # GL_AMBIENT/GL_DIFFUSE/GL_SPECULAR/GL_EMISSION/GL_AMBIENT_AND_DIFFUSE, 1 for
        # GL_SHININESS, 3 for GL_COLOR_INDEXES - verified exhaustively against every
        # third_party/OpenGL-Registry/xml/gl.xml <enum> tag carrying group="MaterialParameter"
        # (the exact group glMaterialfv's own `pname` <param> carries; `face`'s own
        # group="TriangleFace" plays no part in the value count). See gits::opengl2::
        # ComputeMaterialParameterValueCount() (pixelTransferSize.h). IN direction only - see
        # _validate_pointer_override's own direction check. This was part of the concrete
        # "playback skipped unsupported OpenGL command: glMaterialfv" gap (black-screen replay,
        # fixed-function lighting material state never replayed) reported against a real
        # capture.
    POINT_PARAMETER_VALUE_COUNT = 'point_parameter_value_count'  # length_source_value is
        # unused (None); instead the extra `pname_param` field names the sibling `pname` enum
        # param glPointParameterfv's len="COMPSIZE(pname)" value count actually depends on: 3
        # for GL_POINT_DISTANCE_ATTENUATION, 1 for GL_POINT_SIZE_MIN/GL_POINT_SIZE_MAX/
        # GL_POINT_FADE_THRESHOLD_SIZE - verified exhaustively against every
        # third_party/OpenGL-Registry/xml/gl.xml <enum> tag carrying
        # group="PointParameterNameARB" (the exact group glPointParameterfv's own `pname`
        # <param> carries). See gits::opengl2::ComputePointParameterValueCount()
        # (pixelTransferSize.h). IN direction only - see _validate_pointer_override's own
        # direction check.
    VERTEX_ATTRIB_PARAMETER_VALUE_COUNT = 'vertex_attrib_parameter_value_count'  # length_source_
        # value is unused (None); instead the extra `pname_param` field names the sibling
        # `pname` enum param glGetVertexAttribiv's own gl.xml len="4" annotation mis-describes
        # as a FIXED count: gl.xml's len="4" is wrong for the overwhelming
        # majority of this command's own legal pnames. Verified exhaustively against every
        # third_party/OpenGL-Registry/xml/gl.xml <enum> tag carrying group="VertexAttribPropertyARB"
        # (the exact group glGetVertexAttribiv's own `pname` <param> carries) - GL_CURRENT_
        # VERTEX_ATTRIB (0x8626) is the ONLY member that returns 4 values (the generic vertex
        # attribute's own x/y/z/w, not a single scalar array-state property); every other member
        # (GL_VERTEX_ATTRIB_ARRAY_ENABLED/SIZE/STRIDE/TYPE/NORMALIZED/INTEGER/LONG/DIVISOR/
        # BUFFER_BINDING, GL_VERTEX_ATTRIB_BINDING, GL_VERTEX_ATTRIB_RELATIVE_OFFSET) returns
        # exactly one GLint - matching the older, non-OpenGL2 backend's own
        # OpenGL/recorder/openglState.cpp CVariableVertexAttribInfo::Get(), which already reads
        # every one of those single-value pnames into a plain scalar `GLint` destination, never
        # a 4-element one. See gits::opengl2::ComputeVertexAttribParameterValueCount()
        # (pixelTransferSize.h). OUT direction only (unlike TEX_SAMPLER_PARAMETER_VALUE_COUNT/
        # PATCH_PARAMETER_VALUE_COUNT above, both IN-only setters - glGetVertexAttribiv is a
        # getter, so `params` is written by the driver, not read by it) - see
        # _validate_pointer_override's own direction check for this kind. No
        # `tex_sampler_family`-style family discriminator exists or is needed: glGetVertexAttribiv
        # is the only command in this shape's own family (no glGetVertexAttribfv/dv sibling is
        # enabled today, and even if one were, GL_CURRENT_VERTEX_ATTRIB's 4-element arity is the
        # same regardless of the destination's own scalar type).


class TexOrSamplerParameterFamily(StrEnum):
    """Which of the two glTexParameter*v/glSamplerParameter*v command families a
    TEX_SAMPLER_PARAMETER_VALUE_COUNT override belongs to - the Python-side mirror of
    pixelTransferSize.h's gits::opengl2::TexOrSamplerParameterCommand enum. Read that C++
    enum's own comment for the full gl.xml evidence: in short,
    GL_TEXTURE_SWIZZLE_RGBA is a legal 4-element pname for every TEX_PARAMETER form
    (glTexParameterfv/iv/Iiv/Iuiv - all four share gl.xml's group="TextureParameterName" on
    their own `pname`), but not for any SAMPLER_PARAMETER form (glSamplerParameterfv/iv/Iiv/
    Iuiv - group="SamplerParameterF"/"SamplerParameterI", neither of which includes
    GL_TEXTURE_SWIZZLE_RGBA anywhere in the registry - samplers have no swizzle state at all).
    """
    TEX_PARAMETER = 'tex_parameter'
    SAMPLER_PARAMETER = 'sampler_parameter'


@dataclass(frozen=True, kw_only=True)
class PointerParamOverride:
    direction: PointerDirection
    length_source_kind: LengthSourceKind
    length_source_value: str | None = None  # sibling name, fixed-count literal, or None
    length_multiplier: int = 1  # element count = length_source_value's value * length_multiplier
        # - e.g. glUniform4fv's `count` sibling gives *vectors*, not *floats*; gl.xml's own
        # len="count*4" annotation gives the multiplier directly. Unused (stays 1) by every
        # SIBLING_PARAM entry that already counts elements 1:1 with its sibling's value.
    # Only meaningful when length_source_kind is PIXEL_TRANSFER_SIZE - the sibling param names
    # ComputePixelTransferByteSize() needs (see that LengthSourceKind member's own comment).
    # pixel_format_param/pixel_type_param/pixel_width_param are always required;
    # pixel_height_param/pixel_depth_param are optional (a 1D transfer has neither; depth
    # requires height). None for every other kind.
    pixel_format_param: str | None = None
    pixel_type_param: str | None = None
    pixel_width_param: str | None = None
    pixel_height_param: str | None = None
    pixel_depth_param: str | None = None

    # Only meaningful when length_source_kind is CLEAR_BUFFER_VALUE_COUNT - the sibling
    # `buffer` enum param name ComputeClearBufferValueCount() needs. None for every other kind.
    clear_buffer_param: str | None = None

    # Only meaningful when length_source_kind is TEX_SAMPLER_PARAMETER_VALUE_COUNT,
    # PATCH_PARAMETER_VALUE_COUNT, MATERIAL_PARAMETER_VALUE_COUNT, POINT_PARAMETER_VALUE_COUNT,
    # or VERTEX_ATTRIB_PARAMETER_VALUE_COUNT - the sibling `pname` enum param name
    # ComputeTexOrSamplerParameterValueCount()/ComputePatchParameterValueCount()/
    # ComputeMaterialParameterValueCount()/ComputePointParameterValueCount()/
    # ComputeVertexAttribParameterValueCount() needs. None for every other kind.
    pname_param: str | None = None

    # True when this IN-direction
    # parameter's "pointer" is real client memory ONLY when no GL_PIXEL_UNPACK_BUFFER is
    # bound - when one is, GL semantics repurpose the same parameter as a buffer-relative BYTE
    # OFFSET instead (see gits::opengl2::GuardUnpackPixelsPointer(),
    # OpenGL2/recorder/clientMemoryCapture.cpp), and dereferencing it as a pointer is an
    # arbitrary-offset-treated-as-an-address crash risk. PIXEL_TRANSFER_SIZE overrides already
    # get this exact guard unconditionally, in every Mako template that constructs one (see
    # commandsAuto.h.mako/recorderWrappersAuto.cpp.mako's own is_pixel_transfer_in-style
    # checks) - this field exists for every OTHER LengthSourceKind whose parameter has the
    # identical client-pointer-or-PBO-offset duality but a different length formula. So far:
    # SIBLING_PARAM's `data`/`imageSize` shape (glCompressedTexImage*/glCompressedTexSubImage*
    # - see this file's own POINTER_PARAM_OVERRIDES entries for those six commands). Default
    # False (every other SIBLING_PARAM user - glBufferData, glGenTextures's `textures`, ... -
    # is not a pixel-transfer-shaped parameter at all and must not set this). Requires
    # direction IN (see intermediates_creator.py's _validate_pointer_override) - there is no
    # OUT-direction PBO-offset-vs-pointer duality this backend enables today (glReadPixels'
    # `pixels` is already PIXEL_TRANSFER_SIZE, not a candidate for this field at all). Must not
    # be set together with PIXEL_TRANSFER_SIZE itself - redundant with (and potentially
    # confusable against) that kind's own unconditional guard; see
    # _validate_pointer_override's own check forbidding the combination.
    unpack_pbo_guard: bool = False

    # True when this OUT-direction parameter is
    # real, driver-writable client memory ONLY when no GL_QUERY_BUFFER is bound (per
    # ARB_query_buffer_object) - when one is, the driver redirects its result write to that
    # buffer at a byte offset given by this same parameter, instead of writing through it as a
    # pointer, and the eager post-call read this backend performs to capture the result would
    # otherwise treat that small integer offset as an arbitrary address (see
    # gits::opengl2::GuardQueryBufferPointer(), OpenGL2/recorder/clientMemoryCapture.cpp). So
    # far: glGetQueryObjecti64v/glGetQueryObjectui64v/glGetQueryObjectuiv's `params` (every
    # member of QueryObjectParameterName's result-redirection set per
    # ARB_query_buffer_object's own spec text - NOT glGetQueryiv's `params`, which queries
    # scalar, per-target/pname state via QueryParameterName, a completely different enum group
    # ARB_query_buffer_object's result-redirection does not apply to at all). Default False.
    # Requires direction OUT (see intermediates_creator.py's _validate_pointer_override) - a
    # query buffer can only ever be the TARGET of one of these calls, never a source the
    # driver reads existing client memory from.
    query_buffer_guard: bool = False

    # Only meaningful when length_source_kind is TEX_SAMPLER_PARAMETER_VALUE_COUNT - which of
    # the two command families this override's own command belongs to (see
    # TexOrSamplerParameterFamily's own comment above).
    # Required (never None) for that kind - PATCH_PARAMETER_VALUE_COUNT has no family
    # distinction at all (glPatchParameterfv is the only command in that family, no sibling
    # exists to distinguish it from) and must leave this None; see
    # intermediates_creator.py's _validate_pointer_override for the check that enforces both
    # halves of that rule.
    tex_sampler_family: TexOrSamplerParameterFamily | None = None

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
    # glCreateTextures (GL_ARB_direct_state_access/GL 4.5's DSA sibling of glGenTextures - see
    # glCreateQueries' own comment below for why this whole "Create" family had no
    # POINTER_PARAM_OVERRIDES entry at all before this fix, and is a distinct gap from the
    # EXT-alias gap immediately below).
    ('glCreateTextures', 'textures'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    # glGenTexturesEXT/glDeleteTexturesEXT (GL_EXT_texture_object, GL 1.0-era) are byte-for-
    # byte functional duplicates of glGenTextures/glDeleteTextures - same params, same
    # semantics - but gl.xml carries no <alias> element for either (verified directly against
    # third_party/OpenGL-Registry/xml/gl.xml: neither command has an <alias> child, unlike e.g.
    # glGenBuffersARB's <alias name="glGenBuffers"/> below). EXT_texture_object predates the
    # ARB-suffixed-pre-promotion-alias convention gl.xml's <alias> tag models, so
    # _pointer_override_for_command_or_alias's alias fallback (intermediates_creator.py) never
    # applies to these two - they need their own table entry, not inherited coverage, or they
    # fall back to the generic "unsupported command" passthrough with no object-name tracking
    # at all. This was a real, concrete bug: a real target application's captured OpenGL stream
    # calls glGenTexturesEXT for some of its textures (this app leans on ARB/EXT-suffixed entry
    # points throughout - see the matching glGenQueriesEXT/glDeleteQueriesEXT comment below),
    # so every later glDeleteTextures/glBindTexture/etc. reference to one of those names hit
    # ObjectNameMapService's "no registered key" warning - not a startup race or an
    # attach-mid-session gap, just this codegen coverage hole (accounted for the large majority
    # of that stream's Texture-kind warnings).
    ('glGenTexturesEXT', 'textures'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    ('glDeleteTexturesEXT', 'textures'): PointerParamOverride(
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
    # glNamedBufferSubDataEXT aliases glNamedBufferSubData (gl.xml: <alias name=
    # "glNamedBufferSubData"/>) - this entry enables both via _pointer_override_for_command_or_alias's
    # alias fallback, mirroring glBufferSubData's own entry immediately above exactly (same
    # (offset, size, data) shape, just DSA-addressed by buffer name instead of bound target).
    ('glNamedBufferSubData', 'data'): PointerParamOverride(
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

    # ChoosePixelFormat's `pPfd`: the app's REQUESTED PIXELFORMATDESCRIPTOR (an IN struct, not
    # OUT like DescribePixelFormat's own `ppfd` below) - same single-struct FIXED_COUNT shape as
    # SetPixelFormat's `ppfd` above, just a different param name (`pPfd`, capital P) and a
    # different verb (a pure query the app typically feeds into a following SetPixelFormat call,
    # not a mutation) - see opengl2-player-skeleton-plan.md's Decision 9, which originally left
    # this one unsupported alongside the other pixel-format *query* commands (nothing in this
    # backend's replay model consumed a query's result) but is revised by the same real-capture
    # evidence this override responds to: a real application
    # calls plain `ChoosePixelFormat`/`wglChoosePixelFormat`, not just `wglChoosePixelFormatARB`.
    # This is also the SECOND renamed-and-overridden command (`SetPixelFormat` above was the
    # first) - Decision 9's own "Revisit when" names this exact trigger ("a second renamed-and-
    # overridden command ever needs a pointer override") as the point to replace this
    # dual-registration workaround with a real fix in intermediates_creator.py/generator.py
    # instead of one more per-entry pair; not done here to keep this a targeted, minimal-risk
    # addition, but tracked as a real, now-doubly-motivated follow-up rather than deferred
    # speculation.
    ('ChoosePixelFormat', 'pPfd'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('wglChoosePixelFormat', 'pPfd'): PointerParamOverride(
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
    # glDrawElementsBaseVertex(mode, count, type, indices, basevertex): identical `indices` shape
    # to glDrawElements above (gl.xml: same `len="COMPSIZE(count,type)"` const void* pointer) -
    # `basevertex` is a plain GLint scalar needing no override, same as glDrawElementsInstanced-
    # BaseVertex's own `basevertex`. Missing table entry only.
    ('glDrawElementsBaseVertex', 'indices'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN),

    # Legacy fixed-function client vertex-array pointers (glVertexPointer/glColorPointer/
    # glNormalPointer/glTexCoordPointer/glSecondaryColorPointer): gl.xml gives each `pointer`
    # the identical `const void *` + `len="COMPSIZE(...)"` shape as glVertexAttribPointer's own
    # `pointer` above, and the exact same client-memory-pointer-or-GL_ARRAY_BUFFER-relative-
    # byte-offset duality (these predate generic vertex attributes but share GL_ARRAY_BUFFER
    # binding semantics via glBindBuffer since GL 1.5) - same OPAQUE_BIT_PATTERN shape, missing
    # table entries only. `size`/`type`/`stride` on each are plain scalars needing no override.
    ('glVertexPointer', 'pointer'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN),
    ('glColorPointer', 'pointer'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN),
    ('glNormalPointer', 'pointer'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN),
    ('glTexCoordPointer', 'pointer'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN),
    ('glSecondaryColorPointer', 'pointer'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN),

    # glMultiDrawElementsBaseVertex(mode, count, type, indices, drawcount, basevertex): all
    # three array params share the same `len="drawcount"` sibling (gl.xml). `count`/
    # `basevertex` are ordinary GLsizei/GLint arrays - same plain SIBLING_PARAM shape as
    # glGenTextures' `textures` etc. above, nothing new. `indices` is the array-of-opaque-
    # bit-patterns shape - see LengthSourceKind.OPAQUE_BIT_PATTERN_ARRAY's own comment above for
    # why this must not go through ARRAY_OF_ARRAYS_PARAM_OVERRIDES/ArrayOfArrays<T> despite
    # matching its `const void* const*` structural shape.
    ('glMultiDrawElementsBaseVertex', 'count'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='drawcount'),
    ('glMultiDrawElementsBaseVertex', 'indices'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN_ARRAY, length_source_value='drawcount'),
    ('glMultiDrawElementsBaseVertex', 'basevertex'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='drawcount'),

    # Vector/matrix uniform setters (glUniform1fv..4fv/1iv..4iv/1uiv..4uiv,
    # glUniformMatrix2fv/3fv/4fv) USED TO have manual entries here. They are now auto-derived
    # instead, along with every other same-family sibling (glUniform*dv,
    # glProgramUniform*{f,i,ui,d}v, glUniformMatrix*dv, non-square glUniformMatrix{2,3,4}x{2,3,4}
    # {f,d}v, and the glProgramUniformMatrix* counterparts of all of those) - see
    # uniform_setter_metadata.py's derive_uniform_setter_override(), consulted as a fallback by
    # intermediates_creator.py's _is_param_capturable/generator_helpers.py's
    # pointer_override_for() whenever this table has no explicit entry for a `value` param on a
    # command matching that module's own closed family regexes. Removed rather than left in
    # place, both to prove the auto path actually replaces (not just supplements) these 15
    # entries, and per this file's own header comment against speculative/redundant entries -
    # see test_uniform_setter_metadata.py / test_pointer_metadata.py for the regression coverage
    # pinning this down.

    # VAO/FBO array-output creation and array-input destruction - same SIBLING_PARAM('n') shape
    # as glGenTextures/glDeleteTextures above (see
    # docs/development/opengl2-player-skeleton-plan.md's Decision 10).
    ('glGenVertexArrays', 'arrays'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    ('glDeleteVertexArrays', 'arrays'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    # glCreateVertexArrays - see glCreateQueries' own comment below for the "Create" family gap.
    ('glCreateVertexArrays', 'arrays'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    ('glGenFramebuffers', 'framebuffers'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    # glCreateFramebuffers - see glCreateQueries' own comment below for the "Create" family gap.
    ('glCreateFramebuffers', 'framebuffers'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    ('glDeleteFramebuffers', 'framebuffers'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),

    # glInvalidateFramebuffer(target, numAttachments, attachments): `attachments` is
    # len="numAttachments" in gl.xml - a PLAIN sibling reference (not a COMPSIZE(...)
    # expression), same shape as glDrawBuffers' `bufs`/`n` above. `attachments` is a real
    # `const GLenum *` array of attachment-point tokens (GL_COLOR_ATTACHMENTi/GL_DEPTH/
    # GL_STENCIL/GL_DEPTH_STENCIL/GL_COLOR/GL_DEPTH/GL_STENCIL for the default framebuffer,
    # group="InvalidateFramebufferAttachment" in gl.xml) - never a buffer-relative offset or a
    # GLuint object name (no `class` attribute on either `target`/`numAttachments`/
    # `attachments` in gl.xml, verified directly), so it needs no unpack_pbo_guard and no
    # object-name classification, unlike glDrawElements'/pixel-transfer commands' pointer
    # params. `numAttachments` is a plain GLsizei scalar sibling - ordinary SIBLING_PARAM,
    # nothing new. Only glInvalidateFramebuffer itself is added here - its siblings
    # glInvalidateNamedFramebufferData/glInvalidateNamedFramebufferSubData/
    # glInvalidateSubFramebuffer share the identical `attachments`/`numAttachments` shape (and
    # would need the identical entry to enable), but each also has its own additional params
    # (a `framebuffer` GLuint, or x/y/width/height) out of scope for this entry - left for a
    # future round rather than bundled in speculatively.
    ('glInvalidateFramebuffer', 'attachments'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='numAttachments'),

    # glTexImage2D's `pixels` USED TO have a manual PIXEL_TRANSFER_SIZE entry here. It is now
    # auto-derived instead - see pixel_transfer_metadata.py's derive_pixel_transfer_override(),
    # consulted as a fallback by intermediates_creator.py's _is_param_capturable/
    # generator_helpers.py's pointer_override_for() whenever this table has no explicit entry.
    # glTexImage2D's `pixels` (COMPSIZE(format,type,width,height), `const void *`) matches that
    # fallback's shape exactly, so the hand entry became pure redundancy - removed rather than
    # left in place, both to prove the auto path actually replaces (not just supplements) the
    # manual one, and per this file's own header comment against speculative/redundant entries.
    # See test_pixel_transfer_metadata.py / test_intermediates_creator.py for the regression
    # coverage pinning this down. glTexImage1D/3D, glTexSubImage1D/2D/3D, and several other
    # COMPSIZE(format,type,...)-shaped commands across the registry (glDrawPixels,
    # glMultiTexImage*EXT, glTextureImage*EXT, ...) now enable the same way, with zero
    # per-command table entries at all.

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
    # glCreateQueries (GL_ARB_direct_state_access/GL 4.5's DSA sibling of glGenQueries - creates
    # and immediately gives every returned name a real query object of `target`'s type, rather
    # than merely reserving unused names the way glGenQueries does) had NO POINTER_PARAM_OVERRIDES
    # entry at all before this fix - not an alias gap like glGenQueriesEXT/glGenTexturesEXT above
    # (gl.xml gives it no <alias> either way, but it is not a pre-1.1/EXT-era entry point that
    # predates gl.xml's alias convention - it is simply a whole DSA "Create" family this backend
    # never added POINTER_PARAM_OVERRIDES support for at all yet, along with its
    # glCreateTextures/glCreateFramebuffers/glCreateRenderbuffers/glCreateVertexArrays siblings
    # elsewhere in this file). Added on its own merits (completeness with the rest of the
    # already-supported Gen/Delete family for the same namespace), not because it was confirmed
    # to explain any specific observed warning pattern - a real target application's captured
    # stream that motivated this whole investigation had a residual, highly regular block of
    # Query "no registered key" misses whose shape (contiguous runs of exactly 128 names, spaced
    # exactly 256 apart) looked exactly like a single missed N-name batch create, but adding this
    # entry and re-testing against that same stream showed zero change in that specific warning
    # count - that application does not call glCreateQueries at all, so its root cause remains
    # open. Recorded here so a future investigation does not re-suspect this same command
    # without first checking this comment.
    ('glCreateQueries', 'ids'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),

    # glGenQueriesEXT/glDeleteQueriesEXT (GL_EXT_occlusion_query_boolean/GL_EXT_disjoint_timer_
    # query) - same "no <alias> element in gl.xml" gap as glGenTexturesEXT/glDeleteTexturesEXT
    # above (verified directly against third_party/OpenGL-Registry/xml/gl.xml). Note this is
    # deliberately distinct from glGenQueriesARB/glDeleteQueriesARB, which DO carry
    # <alias name="glGenQueries"/>/<alias name="glDeleteQueries"/> and were therefore already
    # enabled via _pointer_override_for_command_or_alias's alias fallback with no table entry
    # of their own - only the EXT-suffixed pair needed one. This was the other half of the same
    # real-target-application capture bug described in the glGenTexturesEXT comment above:
    # that stream's ~4,096 Query-kind "no registered key" warnings - the large majority of that
    # capture's total warning count - trace to this app calling glGenQueriesEXT (not
    # the already-enabled glGenQueriesARB) for its query-object pool.
    ('glGenQueriesEXT', 'ids'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    ('glDeleteQueriesEXT', 'ids'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),

    # glGenRenderbuffers/glDeleteRenderbuffers, glGenSamplers/glDeleteSamplers,
    # glGenProgramPipelines/glDeleteProgramPipelines: same SIBLING_PARAM array-output-creation/
    # array-input-destruction shape as glGenQueries/glGenVertexArrays/glGenFramebuffers above -
    # (see object_name_lifecycle.py's matching
    # OBJECT_NAME_PARAM_OVERRIDES entries) closing a real, previously-silent replay-corruption
    # gap: all three Bind commands (glBindRenderbuffer/glBindSampler/glBindProgramPipeline)
    # were already `enabled=True` (scalar-only) with no lifecycle tracking of any kind for
    # their object namespace, so their GLuint name was passed straight through to the replay
    # driver unresolved. glGenSamplers/glDeleteSamplers's sibling count param is named `count`,
    # not `n` - the other two follow every other Gen/Delete pair's `n` convention.
    ('glGenRenderbuffers', 'renderbuffers'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    # glCreateRenderbuffers - see glCreateQueries' own comment below for the "Create" family gap.
    ('glCreateRenderbuffers', 'renderbuffers'): PointerParamOverride(
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

    # glReadPixels' `pixels` USED TO have a manual PIXEL_TRANSFER_SIZE entry here too (the
    # first OUT-direction user of this kind - recorderWrappersAuto.cpp.mako's post-call
    # Capture() branch computes the byte count the same way commandsAuto.h.mako's IN-direction
    # constructor branch does, rather than assuming a single sibling value). Removed for the
    # same reason as glTexImage2D's above: `void *pixels`, COMPSIZE(format,type,width,height)
    # matches pixel_transfer_metadata.py's fallback derivation (including its OUT-direction
    # case, `param.type == 'void *'`) exactly, so the hand entry was pure redundancy.

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

    # Commands found unsupported by an actual sample run, enabled by following this file's own
    # existing patterns exactly - see each group's own comment for the specific gl.xml evidence.

    # glDeleteBuffers' `buffers`: same SIBLING_PARAM('n') array-input-destruction shape as
    # glDeleteTextures/glDeleteVertexArrays/glDeleteFramebuffers above - glGenBuffers already
    # had this shape's OUT counterpart; glDeleteBuffers itself was simply missing an entry.
    ('glDeleteBuffers', 'buffers'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),

    # glCompressedTex(Sub)Image{1,2,3}D's `data`: len="imageSize" is a PLAIN sibling reference
    # (verified against gl.xml - NOT a COMPSIZE(...) expression at all, unlike the uncompressed
    # Tex(Sub)Image family), so this is an ordinary SIBLING_PARAM entry, structurally identical
    # to glBufferData's `data` above - no new mechanism, and no relationship to
    # pixel_transfer_metadata.py's own fallback (that fallback only ever fires for a
    # COMPSIZE(format, type,...) shape, which this is not). All six 1D/2D/3D Image/SubImage
    # variants share this exact shape, so all six are enabled together.
    #
    # unpack_pbo_guard=True: exactly like glTexImage2D's own `pixels`, `data`
    # is a buffer-relative byte offset rather than real client memory whenever
    # GL_PIXEL_UNPACK_BUFFER is bound - the plain SIBLING_PARAM('imageSize') shape above has no
    # COMPSIZE(...) formula, so it never went through pixel_transfer_metadata.py's fallback and
    # never inherited PIXEL_TRANSFER_SIZE's own unconditional GuardUnpackPixelsPointer() call;
    # this field wires the identical guard onto the SIBLING_PARAM path instead - see this
    # field's own comment (PointerParamOverride.unpack_pbo_guard) and
    # recorderWrappersAuto.cpp.mako's IN-direction construction branch.
    ('glCompressedTexImage1D', 'data'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='imageSize',
        unpack_pbo_guard=True),
    ('glCompressedTexImage2D', 'data'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='imageSize',
        unpack_pbo_guard=True),
    ('glCompressedTexImage3D', 'data'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='imageSize',
        unpack_pbo_guard=True),
    ('glCompressedTexSubImage1D', 'data'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='imageSize',
        unpack_pbo_guard=True),
    ('glCompressedTexSubImage2D', 'data'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='imageSize',
        unpack_pbo_guard=True),
    # glCompressedTextureSubImage2DEXT (GL_EXT_direct_state_access) has no <alias> in gl.xml to
    # any core command (EXT DSA predates ARB/core DSA's differently-shaped equivalent, which
    # drops the redundant `target` param) - needs its own entry, not just an alias fallback.
    # Same len="imageSize" sibling shape as glCompressedTexSubImage2D above; its pointer param
    # is named `bits` instead of `data` (gl.xml's own naming for this one command), and the same
    # GL_PIXEL_UNPACK_BUFFER-binding ambiguity applies regardless of bind-vs-DSA addressing, so
    # unpack_pbo_guard=True carries over unchanged.
    ('glCompressedTextureSubImage2DEXT', 'bits'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='imageSize',
        unpack_pbo_guard=True),
    ('glCompressedTexSubImage3D', 'data'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='imageSize',
        unpack_pbo_guard=True),

    # glGetActiveUniform/glGetActiveAttrib/glGetTransformFeedbackVarying(program, index,
    # bufSize, length, size, type, name): gl.xml gives `length`/`size`/`type` each an explicit
    # len="1" (a real registry annotation here, not an inferred fact like glGetShaderiv's
    # COMPSIZE(pname) override above) and `name` a plain len="bufSize" sibling reference (not
    # COMPSIZE(...) either) - so both halves are exactly what gl.xml already says, with no
    # judgment call needed for the pointer shape. TEXT_OUTPUT_BUFFERS narrows the post-call
    # capture to the reported text plus terminator; bufSize remains the replay capacity.
    ('glGetActiveUniform', 'length'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('glGetActiveUniform', 'size'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('glGetActiveUniform', 'type'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('glGetActiveUniform', 'name'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='bufSize'),
    ('glGetActiveAttrib', 'length'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('glGetActiveAttrib', 'size'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('glGetActiveAttrib', 'type'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('glGetActiveAttrib', 'name'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='bufSize'),
    ('glGetTransformFeedbackVarying', 'length'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('glGetTransformFeedbackVarying', 'size'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('glGetTransformFeedbackVarying', 'type'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('glGetTransformFeedbackVarying', 'name'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='bufSize'),

    # glGetProgramInfoLog/glGetProgramPipelineInfoLog/glGetShaderInfoLog(<object>, bufSize,
    # length, infoLog): same len="1" `length` / len="bufSize" `infoLog` shape as
    # glGetActiveUniform's own `length`/`name` above.
    ('glGetProgramInfoLog', 'length'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('glGetProgramInfoLog', 'infoLog'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='bufSize'),
    ('glGetProgramPipelineInfoLog', 'length'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('glGetProgramPipelineInfoLog', 'infoLog'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='bufSize'),
    ('glGetShaderInfoLog', 'length'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('glGetShaderInfoLog', 'infoLog'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='bufSize'),

    # glGetQueryObjecti64v's `params`: len="COMPSIZE(pname)" mis-describes a variable count -
    # every legal pname (GL_QUERY_RESULT/_AVAILABLE/_NO_WAIT/_TARGET) returns exactly one
    # GLint64, same "COMPSIZE(pname) is a registry-wide annotation convention, not real
    # per-command evidence of a variable count" situation as glGetShaderiv/glGetProgramiv/
    # glGetFramebufferAttachmentParameteriv above - same FIXED_COUNT('1') fix.
    #
    # query_buffer_guard=True: per ARB_query_buffer_object, `params` is a
    # buffer-relative byte offset - not real, driver-writable client memory - whenever
    # GL_QUERY_BUFFER is bound; this backend's own post-call eager read (this OUT param's
    # FIXED_COUNT capture) must not dereference it in that case. See this field's own comment
    # (PointerParamOverride.query_buffer_guard) and
    # gits::opengl2::GuardQueryBufferPointer() (clientMemoryCapture.cpp). Deliberately NOT
    # applied to glGetQueryiv's own `params` above - that command queries scalar,
    # QueryParameterName-grouped state via `target`/`pname` (GL_QUERY_COUNTER_BITS/
    # GL_CURRENT_QUERY), which ARB_query_buffer_object's result-redirection set never includes;
    # only glGetQueryObject{i64,ui64,ui}v's own QueryObjectParameterName-grouped `params`
    # (GL_QUERY_RESULT/_AVAILABLE/_NO_WAIT/_TARGET) is affected.
    ('glGetQueryObjecti64v', 'params'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1',
        query_buffer_guard=True),

    # glGetProgramStageiv's `values`: gl.xml already gives this an explicit len="1" (not even a
    # COMPSIZE(pname) to override - every legal pname really does return exactly one GLint) -
    # simply missing a table entry, not a COMPSIZE mis-description like the group above.
    ('glGetProgramStageiv', 'values'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),

    # glGetProgramPipelineiv's `params`: len="COMPSIZE(pname)" mis-describes a variable count -
    # every legal pname (GL_ACTIVE_PROGRAM/GL_*_SHADER/GL_INFO_LOG_LENGTH/GL_VALIDATE_STATUS)
    # returns exactly one GLint, same fix as glGetProgramiv/glGetQueryObjecti64v above.
    ('glGetProgramPipelineiv', 'params'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),

    # glGetVertexAttribiv's `params`: gl.xml's own len="4" is a registry-wide "generic vertex
    # attribute query, max 4 components" annotation, not real per-pname evidence - it
    # mis-describes the overwhelming majority of this command's own legal
    # pnames, each of which writes exactly ONE GLint (GL_CURRENT_VERTEX_ATTRIB is the sole
    # 4-element exception). A fixed FIXED_COUNT('4') entry (this override's own prior shape)
    # read 3 elements past a caller's valid one-element destination for every other pname - an
    # out-of-bounds read. See LengthSourceKind.VERTEX_ATTRIB_PARAMETER_VALUE_COUNT's own
    # comment (this file) and gits::opengl2::ComputeVertexAttribParameterValueCount()
    # (pixelTransferSize.h) for the pname-aware replacement and its own exhaustive gl.xml
    # audit.
    ('glGetVertexAttribiv', 'params'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.VERTEX_ATTRIB_PARAMETER_VALUE_COUNT,
        pname_param='pname'),

    # glViewportIndexedfv's `v`: gl.xml already gives this an explicit len="4" (x, y, w, h) -
    # simply missing a table entry.
    ('glViewportIndexedfv', 'v'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='4'),

    # glTexParameterfv/glTexParameteriv/glSamplerParameterfv's `params`/`param` USED TO have
    # manual entries here (len="COMPSIZE(pname)" - unlike glGetShaderiv's misleading
    # COMPSIZE(pname), this one really is pname-dependent: 4 values for
    # GL_TEXTURE_BORDER_COLOR, and for the Tex- but not Sampler- variants
    # GL_TEXTURE_SWIZZLE_RGBA; 1 for every other legal pname). They are now auto-derived
    # instead, along with every other same-family sibling (glTexParameterIiv/Iuiv,
    # glSamplerParameteriv/Iiv/Iuiv) - see tex_sampler_parameter_metadata.py's
    # derive_tex_sampler_parameter_override(), consulted as a fallback by
    # intermediates_creator.py's _is_param_capturable/generator_helpers.py's
    # pointer_override_for() whenever this table has no explicit entry for the (command, param)
    # pair. Removed rather than left in place, both to prove the auto path actually replaces
    # (not just supplements) these 3 entries, and per this file's own header comment against
    # speculative/redundant entries - see test_tex_sampler_parameter_metadata.py for the
    # regression coverage pinning this down.

    # glPatchParameterfv's `values`: len="COMPSIZE(pname)", genuinely pname-dependent (4 for
    # GL_PATCH_DEFAULT_OUTER_LEVEL, 2 for GL_PATCH_DEFAULT_INNER_LEVEL - the only two legal
    # pnames) - see LengthSourceKind.PATCH_PARAMETER_VALUE_COUNT's own comment and
    # gits::opengl2::ComputePatchParameterValueCount() (pixelTransferSize.h).
    ('glPatchParameterfv', 'values'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.PATCH_PARAMETER_VALUE_COUNT, pname_param='pname'),

    # glMaterialfv's `params`: len="COMPSIZE(pname)", genuinely pname-dependent (4 for
    # GL_AMBIENT/GL_DIFFUSE/GL_SPECULAR/GL_EMISSION/GL_AMBIENT_AND_DIFFUSE, 1 for
    # GL_SHININESS, 3 for GL_COLOR_INDEXES) - see
    # LengthSourceKind.MATERIAL_PARAMETER_VALUE_COUNT's own comment and gits::opengl2::
    # ComputeMaterialParameterValueCount() (pixelTransferSize.h). `face` (group="TriangleFace")
    # is a plain scalar sibling playing no part in the value count, needing no override.
    ('glMaterialfv', 'params'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.MATERIAL_PARAMETER_VALUE_COUNT, pname_param='pname'),

    # glPointParameterfv's `params`: len="COMPSIZE(pname)", genuinely pname-dependent (3 for
    # GL_POINT_DISTANCE_ATTENUATION, 1 for GL_POINT_SIZE_MIN/GL_POINT_SIZE_MAX/
    # GL_POINT_FADE_THRESHOLD_SIZE) - see LengthSourceKind.POINT_PARAMETER_VALUE_COUNT's own
    # comment and gits::opengl2::ComputePointParameterValueCount() (pixelTransferSize.h).
    ('glPointParameterfv', 'params'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.POINT_PARAMETER_VALUE_COUNT, pname_param='pname'),

    # glUniform4dv/glUniformMatrix4dv/glProgramUniform4dv/glProgramUniformMatrix4dv/
    # glProgramUniformMatrix4fv's `value` USED TO have manual entries here too (the
    # GLdouble-typed classic-uniform and DSA (`glProgramUniform*`) variants). Also now covered
    # by uniform_setter_metadata.py's derive_uniform_setter_override() - see this file's own
    # comment above the removed glUniform1fv..glUniformMatrix4fv entries for why.

    # glUniformSubroutinesuiv's `indices`: len="count" - plain SIBLING_PARAM, same shape as
    # glGenTextures' `textures` etc., just missing a table entry.
    ('glUniformSubroutinesuiv', 'indices'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='count'),

    # glVertexAttribIPointer/glVertexAttribLPointer's `pointer`: no len= attribute at all (a
    # buffer-relative offset or client-memory address, exactly like glVertexAttribPointer's own
    # `pointer` above) - same OPAQUE_BIT_PATTERN shape, just the integer (I) and double (L)
    # generic vertex attribute variants, missing table entries only.
    ('glVertexAttribIPointer', 'pointer'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN),
    ('glVertexAttribLPointer', 'pointer'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN),

    # glDrawElementsIndirect's `indirect`: no len= at all - and, unlike glDrawElements'
    # client-memory-or-offset duality, core GL (4.0+) REQUIRES a buffer bound to
    # GL_DRAW_INDIRECT_BUFFER for this call (spec: an error is generated otherwise) - `indirect`
    # is therefore ALWAYS a buffer-relative byte offset, never a genuine client-memory pointer,
    # so this is unambiguously OPAQUE_BIT_PATTERN, not merely "same shape as glDrawElements by
    # analogy".
    ('glDrawElementsIndirect', 'indirect'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN),

    # glDrawElementsInstancedBaseVertexBaseInstance's `indices`: len="count" structurally, but
    # (like glDrawElements/glDrawElementsInstanced/glDrawElementsInstancedBaseVertex above) the
    # real semantics are the client-memory-pointer-or-GL_ELEMENT_ARRAY_BUFFER-relative-offset
    # duality that len= cannot express - same OPAQUE_BIT_PATTERN treatment as those three,
    # ignoring the len="count" annotation entirely, exactly as they already do.
    ('glDrawElementsInstancedBaseVertexBaseInstance', 'indices'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.OPAQUE_BIT_PATTERN),

    # glGetSubroutineIndex/glGetSubroutineUniformLocation/glGetAttribLocation's `name`: no len=
    # at all - a plain null-terminated GLchar*, identical shape to glGetUniformLocation/
    # glBindAttribLocation's own `name` above, just missing table entries.
    ('glGetSubroutineIndex', 'name'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.NULL_TERMINATED),
    ('glGetSubroutineUniformLocation', 'name'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.NULL_TERMINATED),
    ('glGetAttribLocation', 'name'): PointerParamOverride(
        direction=PointerDirection.IN, length_source_kind=LengthSourceKind.NULL_TERMINATED),

    # glGenTransformFeedbacks/glDeleteTransformFeedbacks' `ids`: same SIBLING_PARAM('n')
    # array-output-creation/array-input-destruction shape as glGenQueries/glGenVertexArrays/
    # glGenFramebuffers above - the pointer-shape half of enabling the whole transform-feedback
    # object family; see object_name_lifecycle.py's GLObjectKind.TRANSFORM_FEEDBACK for the
    # object-name half (which is also what auto-enables glBindTransformFeedback/
    # glIsTransformFeedback/glDrawTransformFeedback(Stream(Instanced)) with NO entries here at
    # all - none of those have an array parameter, just a scalar `id`).
    ('glGenTransformFeedbacks', 'ids'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),
    ('glDeleteTransformFeedbacks', 'ids'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),

    # glDrawBuffers' `bufs`: len="n" is a PLAIN sibling reference (not a COMPSIZE(...)
    # expression) - ordinary SIBLING_PARAM, structurally identical to glGenTextures' `textures`
    # etc. above, just missing a table entry.
    ('glDrawBuffers', 'bufs'): PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='n'),

    # glGetActiveUniformBlockName(program, uniformBlockIndex, bufSize, length,
    # uniformBlockName)/glGetActiveUniformName(program, uniformIndex, bufSize, length,
    # uniformName): gl.xml gives `length` an explicit len="1" and `uniformBlockName`/
    # `uniformName` a plain len="bufSize" sibling reference (neither is a COMPSIZE(...)
    # expression) - the exact same shape as glGetActiveUniform/glGetActiveAttrib/
    # glGetProgramInfoLog's own `length`/name-buffer pair above, just two more OUT string
    # getters missing table entries. TEXT_OUTPUT_BUFFERS uses their reported length after
    # dispatch while preserving bufSize for replay.
    ('glGetActiveUniformBlockName', 'length'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('glGetActiveUniformBlockName', 'uniformBlockName'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='bufSize'),
    ('glGetActiveUniformName', 'length'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),
    ('glGetActiveUniformName', 'uniformName'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.SIBLING_PARAM, length_source_value='bufSize'),

    # glGetQueryiv's `params`: len="COMPSIZE(pname)" - but unlike glGetIntegerv's own
    # COMPSIZE(pname) (genuinely 1/2/4/16-element, no safe LengthSourceKind - see that
    # command's own "stays disabled" test), QueryParameterName's only two legal pname values
    # (GL_QUERY_COUNTER_BITS, GL_CURRENT_QUERY) each return exactly one GLint - same
    # "COMPSIZE(pname) is a registry-wide annotation convention, not real per-command evidence
    # of a variable count" situation as glGetShaderiv/glGetProgramiv/
    # glGetFramebufferAttachmentParameteriv/glGetQueryObjecti64v above - same FIXED_COUNT('1')
    # fix, verified against every QueryParameterName-grouped enum in gl.xml.
    ('glGetQueryiv', 'params'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1'),

    # glGetQueryObjectui64v/glGetQueryObjectuiv's `params`: len="COMPSIZE(pname)", same
    # QueryObjectParameterName group as glGetQueryObjecti64v above (GL_QUERY_RESULT/
    # _AVAILABLE/_NO_WAIT/_TARGET, every one a single value) - just the GLuint64/GLuint result
    # types, same FIXED_COUNT('1') fix, missing table entries only. query_buffer_guard=True for
    # the same ARB_query_buffer_object hazard as glGetQueryObjecti64v above - see that entry's
    # own comment and PointerParamOverride.query_buffer_guard's.
    ('glGetQueryObjectui64v', 'params'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1',
        query_buffer_guard=True),
    ('glGetQueryObjectuiv', 'params'): PointerParamOverride(
        direction=PointerDirection.OUT,
        length_source_kind=LengthSourceKind.FIXED_COUNT, length_source_value='1',
        query_buffer_guard=True),
}
