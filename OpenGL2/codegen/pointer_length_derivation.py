#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Generic, gl.xml `len=`-driven auto-derivation of pointer/array parameters' LENGTH-SOURCE
# metadata - the len=-based counterpart of object_name_lifecycle.py's class=-based
# auto-derivation. Read that module's own header comment first; this one only explains what
# differs, not the whole "auto-derive from evidence, fall back to a hand table, never guess"
# philosophy both modules share.
#
# THE KEY DIFFERENCE FROM class=: gl.xml's <param class="..."> attribute genuinely *is* the
# answer object_name_lifecycle.py needs (a namespace name, verbatim) - that module's whole
# insight is noticing that fact. gl.xml's <param len="..."> attribute is NOT the same kind of
# signal, for pointer_metadata.py's classification. Audited exhaustively across every <param
# len="..."> in gl.xml (the only one of the four registries that ever sets len= - see
# intermediates_creator.py's find_unknown_command_attributes docstring table) - 2,007 pointer
# params, 91% (1,832) carrying a len= attribute - and confirmed:
#   - len= NEVER encodes direction (IN vs OUT). A `len="count"` annotation looks structurally
#     identical whether the driver reads from that pointer (glUniform4fv's `value`) or writes
#     into it (glGetProgramiv's `params`, which instead carries len="COMPSIZE(pname)" - but
#     other real OUT arrays, like wglGetPixelFormatAttribivARB's `piValues`, carry an ordinary
#     len="nAttributes" sibling reference exactly like an IN array would).
#   - len= NEVER encodes whether the pointer is genuinely dereferenced at all, as opposed to an
#     opaque bit pattern (a buffer-relative byte offset in core-profile GL) that must never be
#     dereferenced. glDrawElements' `indices` (len="COMPSIZE(count,type)") and
#     glMultiDrawElementsBaseVertex's `indices` (len="drawcount") both look, from len= alone,
#     like ordinary arrays with a discoverable count - but pointer_metadata.py classifies both
#     OPAQUE_BIT_PATTERN(_ARRAY), never ArrayArgument<T>/ArrayOfArrays<T> - see that module's own
#     comments on those two LengthSourceKind members. glShaderSource's `string`
#     (len="count", `const GLchar* const*`) has the exact same structural shape as
#     glMultiDrawElementsBaseVertex's `indices` (len="drawcount", `const void* const*`) yet
#     opposite dereference semantics (copy the pointee vs. never touch it) - no len=-visible
#     distinguisher exists between them.
#   - COMPSIZE(...) (582 of the 1,832, 32%) is not a formula at all - it is a parenthesized
#     dependency list naming the *inputs* a real formula would need (pixel-transfer byte size,
#     a `pname`-dependent getter result count, ...), never the count itself. Some hand-authored
#     pointer_metadata.py entries deliberately DISAGREE with what a literal reading of
#     COMPSIZE(...) would imply - e.g. glGetShaderiv.params is len="COMPSIZE(pname)" (implying a
#     variable count) but the GL/GLES spec guarantees exactly one GLint regardless of pname, so
#     the hand entry is FIXED_COUNT('1'), not "whatever COMPSIZE(pname) would suggest". Trusting
#     COMPSIZE(...) literally would silently overwrite a deliberately-correct override with a
#     wrong one.
# So, unlike resolve_object_name_override() (which really does produce a complete (kind, role)
# classification for a previously-unclassified field), this module NEVER derives a complete
# PointerParamOverride/ArrayOfArraysOverride on its own - it only ever derives the LENGTH-SOURCE
# half of that classification (which sibling parameter, or what fixed literal count - see
# LengthSourceDerivation below). The other half - direction, and whether the pointer is a real
# array to copy vs. an opaque bit pattern vs. a pixel-transfer/clear-value byte count formula -
# is a *category* fact len= cannot express at all, for any command, and stays exactly as
# hand-maintained as pointer_metadata.py's own header comment already says it must.
#
# This module has two consumers (see each function's own docstring):
#   1. find_pointer_length_mismatches() - a cross-validation/lint pass, wired into codegen
#      (generator.py) at generation time, that warns if an EXISTING hand-authored
#      POINTER_PARAM_OVERRIDES/ARRAY_OF_ARRAYS_PARAM_OVERRIDES entry's length-source disagrees
#      with what gl.xml's len= would derive for it. This is pure lint - it can never change
#      generated code, only print a warning - so it is safe regardless of how much of the rest
#      of this module ever gets used for anything else. Run once, at introduction, against every
#      existing entry in both tables: zero disagreements (see this module's own test suite's
#      end-to-end test, which pins that result down as a regression check).
#   2. derive_pointer_param_override() - the generic classification for params with no hand
#      entry. The length source comes from len=; direction and "real array vs opaque offset"
#      come from the declared C type (const-qualification, typed element vs void), which len=
#      cannot express - see that function's docstring for the exact rule and its audited
#      exceptions. Consulted last by intermediates_creator.py's _is_param_capturable and, in
#      lockstep, generator_helpers.py's pointer_override_for(), so a command's generated code
#      and its `enabled` gate always agree on which override applies.

import re
from dataclasses import dataclass

from array_of_arrays_metadata import ARRAY_OF_ARRAYS_PARAM_OVERRIDES
from intermediates import Api, Command, Parameter
from object_name_lifecycle import ObjectNameRole, resolve_object_name_override
from pointer_metadata import (
    POINTER_PARAM_OVERRIDES, LengthSourceKind, PointerDirection, PointerParamOverride,
)

try:
    from enum import StrEnum
except ImportError:
    from enum import Enum
    class StrEnum(str, Enum):
        def __str__(self) -> str:
            return self.value


class LenAttribShape(StrEnum):
    """How gl.xml's raw <param len="..."> string is shaped - see classify_len_attrib().

    Every shape actually present in gl.xml today is covered here (verified exhaustively - see
    classify_len_attrib()'s own docstring); a shape this enum has never seen raises rather than
    silently falling into one of these buckets - see that function's own ValueError.
    """
    NONE = 'none'                # no len= attribute at all - 578 of 2,410 pointer params across
                                  # all four registries (gl/wgl/glx/egl - only gl.xml ever sets
                                  # len= at all, so this bucket also covers every wgl/glx/egl
                                  # pointer param unconditionally). No signal whatsoever - could
                                  # be a null-terminated string, a single-element out-param, an
                                  # opaque bit pattern, anything.
    INTEGER = 'integer'          # a literal element count, e.g. len="4". 646 occurrences, all
                                  # in gl.xml.
    PARAM_NAME = 'param_name'    # a plain sibling parameter name, e.g. len="size"
                                  # (glBufferData). 418 occurrences.
    SCALED_PARAM_NAME = 'scaled_param_name'  # sibling * literal, e.g. len="count*4"
                                  # (glUniform4fv). 168 occurrences. The sibling counts *logical*
                                  # elements (vectors, matrices) the caller genuinely provided;
                                  # each is `literal` primitives wide - exactly
                                  # pointer_metadata.py's own length_multiplier field, just
                                  # spelled out by gl.xml itself instead of hand-computed. See
                                  # classify_len_attrib()'s own comment for why this is safe.
    DIVIDED_PARAM_NAME = 'divided_param_name'  # sibling / literal, e.g. len="bufSize / 4"
                                  # (glGetnUniformfv). 18 occurrences. NOT safe - see
                                  # classify_len_attrib()'s own comment for the GL-specific
                                  # gotcha that excludes this shape specifically (unlike
                                  # SCALED_PARAM_NAME's `*` form, which is safe).
    COMPSIZE = 'compsize'         # COMPSIZE(...) - a dependency list, not a formula. 582
                                  # occurrences. NEVER safe to read as a length by itself - see
                                  # this module's own header comment (glGetShaderiv.params).


# --- classify_len_attrib() shape patterns ------------------------------------------------
#
# A gl.xml <param> identifier is always a C identifier (letters/digits/underscore, not starting
# with a digit) - same rule normalize_type()/reconstruct_type_from_xml_elem() already rely on
# for every other identifier this generator reads out of the XML.
_INTEGER_RE = re.compile(r'^-?\d+$')
_PARAM_NAME_RE = re.compile(r'^[A-Za-z_][A-Za-z0-9_]*$')
_SCALED_PARAM_NAME_RE = re.compile(r'^([A-Za-z_][A-Za-z0-9_]*)\s*\*\s*(\d+)$')
_DIVIDED_PARAM_NAME_RE = re.compile(r'^([A-Za-z_][A-Za-z0-9_]*)\s*/\s*(\d+)$')
_COMPSIZE_RE = re.compile(r'^COMPSIZE\(.*\)$')


def classify_len_attrib(len_attrib: str | None) -> LenAttribShape:
    """Classify the shape of a raw gl.xml <param len="..."> string (Parameter.len) - see
    LenAttribShape's own member comments for exactly what each shape means and how many times
    it occurs in the registries this generator reads (all in gl.xml - wgl.xml/glx.xml/egl.xml
    never set len= at all, so every one of their pointer params classifies as NONE).

    Verified exhaustive against every len= value actually present in the checked-out
    third_party/OpenGL-Registry/xml/gl.xml as of this module's introduction (2,410 pointer
    params across all four registries; 1,832 gl.xml ones carry a len= attribute, every single
    one matching exactly one of _INTEGER_RE/_PARAM_NAME_RE/_SCALED_PARAM_NAME_RE/
    _DIVIDED_PARAM_NAME_RE/_COMPSIZE_RE below). Raises, rather than silently guessing a bucket,
    for a shape none of those patterns match - mirroring resolve_object_name_override()'s own
    "fail loud on an unrecognized registry value" precedent (see that function's KeyError) - so
    a future gl.xml revision introducing a genuinely new len= grammar gets noticed immediately
    instead of silently landing in the wrong (or no) bucket.

    On SCALED_PARAM_NAME ('sibling*N') vs DIVIDED_PARAM_NAME ('sibling/N') being safe vs unsafe:
    both are simple arithmetic over a sibling parameter, but they occur on structurally different
    kinds of commands, one of which turns out to have a genuine GL-specific gotcha this
    classification exists to catch (the whole 'other'/arithmetic len= bucket had to be audited
    for exactly this, per the investigation that introduced this module, not merely
    pattern-matched):
      - Every '*' occurrence (glUniform4fv-style vector/matrix setters, glProgramUniform*,
        glVertexAttribs*NV, ...) is an IN parameter where the sibling ('count') is the number of
        logical elements the CALLER is actually passing THIS CALL - the same value
        pointer_metadata.py's own hand-authored SIBLING_PARAM+length_multiplier entries for
        glUniform1fv..glUniformMatrix4fv already use (gl.xml's len="count*4" and that hand
        entry's length_multiplier=4 for glUniform4fv are the literal same fact, one written by
        Khronos, one written by hand - this module's whole purpose is closing exactly that kind
        of redundancy).
      - Every '/' occurrence (glGetnUniformfv/glGetnUniformdv/glGetnPixelMapfvARB/glGetnMapdvARB/
        glGetPerfMonitorCounterDataAMD - every one a KHR_robustness "n"-suffixed getter or
        counter-query, all OUT) has a sibling ('bufSize'/'dataSize') that is the CALLER-SUPPLIED
        CAPACITY of the OUT buffer - a safety upper bound the driver must not write past, per the
        GL_KHR_robustness spec's own "if the ... buffer ... is not large enough ... an error is
        generated" wording - not the number of elements the driver actually writes on this
        particular call (that real count is query/pname-dependent, exactly like glGetIntegerv's
        own pname-dependent count that keeps IT disabled too, and is not recoverable from any
        sibling at all). Treating bufSize/4 as the true written-element count would read past
        whatever the driver actually initialized whenever the real answer is smaller than the
        buffer's capacity - not merely imprecise, genuinely uninitialized-memory-adjacent. This
        is the one 'other'/arithmetic shape that turned out to hide a real, GL-specific gotcha,
        exactly the kind the investigation that introduced this module called for confirming
        before trusting any arithmetic len= form.
    """
    if len_attrib is None:
        return LenAttribShape.NONE
    if _COMPSIZE_RE.match(len_attrib):
        return LenAttribShape.COMPSIZE
    if _INTEGER_RE.match(len_attrib):
        return LenAttribShape.INTEGER
    if _PARAM_NAME_RE.match(len_attrib):
        return LenAttribShape.PARAM_NAME
    if _SCALED_PARAM_NAME_RE.match(len_attrib):
        return LenAttribShape.SCALED_PARAM_NAME
    if _DIVIDED_PARAM_NAME_RE.match(len_attrib):
        return LenAttribShape.DIVIDED_PARAM_NAME
    raise ValueError(
        f'Unrecognized gl.xml len="{len_attrib}" shape - add it to '
        f'pointer_length_derivation.py\'s classify_len_attrib (and to LenAttribShape) if this '
        f'is a genuinely new, verified-safe-or-unsafe pattern; the registries grew a len= '
        f'grammar this module has never audited'
    )


@dataclass(frozen=True, kw_only=True)
class LengthSourceDerivation:
    """The length-SOURCE half of a pointer/array parameter's classification, auto-derived from
    gl.xml's own len= attribute - deliberately NOT a full PointerParamOverride (no `direction`
    field: direction is never derivable from len= alone - see this module's header comment).

    Shape mirrors PointerParamOverride's own length_source_kind/length_source_value/
    length_multiplier fields exactly (on purpose - see find_pointer_length_mismatches(), which
    compares one of these against a real PointerParamOverride field-by-field), so a caller that
    already knows the category from some other safe signal only needs to supply `direction` (and
    the array-element-type/object-name checks intermediates_creator.py already runs) to build a
    complete override - see derive_pointer_param_override()'s own docstring for why nothing in
    this codebase currently has such a safe signal to supply.
    """
    kind: LengthSourceKind  # always FIXED_COUNT or SIBLING_PARAM - see LenAttribShape's members
        # for why every other LengthSourceKind (NULL_TERMINATED, KEY_VALUE_LIST,
        # OPAQUE_BIT_PATTERN, OPAQUE_BIT_PATTERN_ARRAY, PIXEL_TRANSFER_SIZE,
        # CLEAR_BUFFER_VALUE_COUNT) encodes a *category* fact, not a length-source fact, and so
        # is never something this module derives.
    value: str | None  # sibling parameter name (SIBLING_PARAM) or a literal count string
        # (FIXED_COUNT).
    multiplier: int = 1  # only ever > 1 for a SCALED_PARAM_NAME ('sibling*N') len= shape.


def derive_length_source(
    param: Parameter, other_params: list[Parameter],
) -> LengthSourceDerivation | None:
    """Auto-derive the length-SOURCE half of `param`'s classification from gl.xml's own len=
    attribute (param.len), or None if no safe derivation exists - None is always the safe
    answer; this function never guesses (see this module's header comment).

    `other_params` is every other parameter on the same command (mirrors
    _validate_pointer_override's own `other_params` convention in intermediates_creator.py) -
    needed to resolve a SIBLING_PARAM/SCALED_PARAM_NAME's named sibling and confirm it is a
    plain scalar, exactly like _validate_pointer_override already does for a hand-authored
    SIBLING_PARAM entry. This check is load-bearing, not defensive boilerplate: exactly one
    pointer param across all four registries names a len= sibling that fails it -
    glExtGetProgramBinarySourceQCOM's `source` (len="length"), whose sibling `length` is itself
    an OUT `GLint *`, not a by-value scalar (the real count is discovered by writing into
    `length`, an entirely different shape from every other PARAM_NAME occurrence) - correctly
    excluded here, not merely a shape this audit happened not to need.
    """
    shape = classify_len_attrib(param.len)

    if shape is LenAttribShape.INTEGER:
        assert param.len is not None
        return LengthSourceDerivation(kind=LengthSourceKind.FIXED_COUNT, value=param.len)

    if shape is LenAttribShape.PARAM_NAME:
        assert param.len is not None
        sibling = next((p for p in other_params if p.name == param.len), None)
        if sibling is None or not sibling.type_facts.is_scalar:
            return None
        return LengthSourceDerivation(kind=LengthSourceKind.SIBLING_PARAM, value=param.len)

    if shape is LenAttribShape.SCALED_PARAM_NAME:
        assert param.len is not None
        match = _SCALED_PARAM_NAME_RE.match(param.len)
        assert match is not None
        sibling_name, multiplier_text = match.group(1), match.group(2)
        sibling = next((p for p in other_params if p.name == sibling_name), None)
        if sibling is None or not sibling.type_facts.is_scalar:
            return None
        return LengthSourceDerivation(
            kind=LengthSourceKind.SIBLING_PARAM, value=sibling_name,
            multiplier=int(multiplier_text))

    # DIVIDED_PARAM_NAME, COMPSIZE, NONE all fall through to the same "no safe signal" answer -
    # see classify_len_attrib()'s own docstring (DIVIDED_PARAM_NAME) and this module's header
    # comment (COMPSIZE) for why each is excluded, not merely unimplemented.
    return None


# Commands the generic rule in derive_pointer_param_override() would classify WRONGLY - found by
# auditing every command it newly enables. Each needs a hand entry (or stays disabled).
_GENERIC_DERIVATION_DENYLIST: frozenset[str] = frozenset({
    # IN arrays gl.xml declares without `const` - the const rule would call them OUT.
    'glCullParameterdvEXT', 'glCullParameterfvEXT',
    'glDeletePerfMonitorsAMD', 'glSelectPerfMonitorCountersAMD',
    # The driver keeps the pointer and writes into it on LATER calls (glRenderMode, ...); the
    # player's per-call scratch buffer would be freed by then.
    'glFeedbackBuffer', 'glSelectBuffer', 'glInstrumentsBufferSGIX',
    # Semaphore object names - a namespace this backend has no object-name mapping for.
    'glAsyncCopyBufferSubDataNVX', 'glAsyncCopyImageSubDataNVX',
    'glClientWaitSemaphoreui64NVX', 'glSignalSemaphoreui64NVX', 'glWaitSemaphoreui64NVX',
    # `const void *` with a sibling length that is not client data: an element-array-buffer
    # offset (len="count" counts indices, not bytes), and memory ranges the driver retains.
    'glDrawElementsInstancedBaseInstance', 'glDrawElementsInstancedBaseInstanceEXT',
    'glPixelDataRangeNV', 'glTextureRangeAPPLE',
    # Bindless texture handles / GPU addresses: driver-assigned 64-bit values with no replay-time
    # mapping, so replaying the captured values would reference the wrong memory.
    'glUniformHandleui64vARB', 'glProgramUniformHandleui64vARB',
    'glUniformHandleui64vIMG', 'glProgramUniformHandleui64vIMG',
    'glUniformHandleui64vNV', 'glProgramUniformHandleui64vNV',
    'glUniformui64vNV', 'glProgramUniformui64vNV',
    # Debug-message filter: deliberately not captured, see the architecture page's "Reduce
    # replay warning noise" decision.
    'glDebugMessageControl', 'glDebugMessageControlARB', 'glDebugMessageControlKHR',
    'glDebugMessageEnableAMD',
})

_GEN_CREATE_DELETE_RE = re.compile(r'^gl(?:Gen|Create|Delete)(?:$|[A-Z])')


def derive_pointer_param_override(
    command_name: str, param: Parameter, other_params: list[Parameter],
) -> PointerParamOverride | None:
    """Generic classification for a pointer param with no POINTER_PARAM_OVERRIDES entry (and no
    narrower family derivation), or None. Consulted last by intermediates_creator.py's
    _is_param_capturable and generator_helpers.py's pointer_override_for().

    len= alone never encodes direction or "real array vs opaque offset" (see this module's
    header comment). The declared C type does, for the two shapes accepted here:
      - A single-level pointer to a typed element (GLfloat*, GLuint*, ...; never void*) whose
        len= derives a length source (a literal or a sibling count). A typed element is never
        an opaque buffer offset; `const T*` is read by the driver (IN), `T*` is written (OUT).
      - `const void *` whose len= is a plain sibling byte count (glNamedBufferData's `data`,
        glProgramBinary's `binary`, glCompressed*Image*'s `data`/`bits`). `imageSize` siblings
        are compressed pixel uploads and get the unpack-PBO guard. OUT void* is never derived:
        those are image/buffer read-backs whose pointer can be a pack-buffer offset.
    Exceptions found by auditing every newly enabled command are in
    _GENERIC_DERIVATION_DENYLIST. Two object-name shapes are also refused:
      - An existing-object name array written by the driver (glGetAttachedShaders): object-name
        resolution runs before the call, when the array is not filled yet.
      - A Gen/Create/Delete name array gl.xml does not tag with class= (NV fences, AMD perf
        monitors, ...): a namespace without object-name mapping would replay raw names.
    """
    if command_name in _GENERIC_DERIVATION_DENYLIST:
        return None
    facts = param.type_facts
    if facts.pointer_depth != 1:
        return None
    derived = derive_length_source(param, other_params)
    if derived is None:
        return None

    if facts.base_type == 'void':
        if param.type != 'const void *' or derived.kind is not LengthSourceKind.SIBLING_PARAM:
            return None
        return PointerParamOverride(
            direction=PointerDirection.IN,
            length_source_kind=LengthSourceKind.SIBLING_PARAM,
            length_source_value=derived.value,
            length_multiplier=derived.multiplier,
            unpack_pbo_guard=derived.value == 'imageSize')

    if not facts.is_array_element_eligible:
        return None
    direction = PointerDirection.IN if param.type.startswith('const ') else PointerDirection.OUT

    object_name = resolve_object_name_override(command_name, param.name, param.class_)
    if object_name is not None:
        if direction is PointerDirection.OUT and object_name.role in (
                ObjectNameRole.LOOKUP, ObjectNameRole.LOOKUP_OR_CREATE):
            return None
    elif _GEN_CREATE_DELETE_RE.match(command_name) and facts.base_type in ('GLuint', 'GLint'):
        return None

    return PointerParamOverride(
        direction=direction,
        length_source_kind=derived.kind,
        length_source_value=derived.value,
        length_multiplier=derived.multiplier)


def _hand_entry_as_length_source(
    length_source_kind: LengthSourceKind, length_source_value: str | None, length_multiplier: int,
) -> tuple[LengthSourceKind, str | None, int] | None:
    """Normalize a hand-authored PointerParamOverride's length-source fields into the same
    (kind, value, multiplier) shape derive_length_source() produces, for
    find_pointer_length_mismatches()'s comparison - or None if this LengthSourceKind encodes a
    category fact with nothing length-source-shaped to compare (see LengthSourceDerivation's own
    docstring for why FIXED_COUNT/SIBLING_PARAM are the only two kinds this module ever derives).

    OPAQUE_BIT_PATTERN_ARRAY is deliberately normalized to SIBLING_PARAM here, not skipped: per
    pointer_metadata.py's own comment on that LengthSourceKind ("same sibling-length shape/
    validation as SIBLING_PARAM above ... length_source_value names the sibling drawcount-style
    param, exactly like SIBLING_PARAM"), it uses the identical length-DISCOVERY mechanism as
    SIBLING_PARAM - only the ELEMENT treatment differs (never dereferenced vs. copied), which is
    exactly the category fact this module cannot and does not compare. Comparing its
    length_source_value/length_multiplier against gl.xml's len= is exactly as meaningful as for
    a real SIBLING_PARAM entry (see glMultiDrawElementsBaseVertex.indices, len="drawcount",
    OPAQUE_BIT_PATTERN_ARRAY(length_source_value='drawcount') - a real, checked example, not a
    hypothetical one).
    """
    if length_source_kind in (LengthSourceKind.FIXED_COUNT, LengthSourceKind.SIBLING_PARAM):
        return (length_source_kind, length_source_value, length_multiplier)
    if length_source_kind is LengthSourceKind.OPAQUE_BIT_PATTERN_ARRAY:
        return (LengthSourceKind.SIBLING_PARAM, length_source_value, length_multiplier)
    # NULL_TERMINATED/KEY_VALUE_LIST/OPAQUE_BIT_PATTERN/PIXEL_TRANSFER_SIZE/
    # CLEAR_BUFFER_VALUE_COUNT: pure category kinds with no comparable length-source shape
    # (OPAQUE_BIT_PATTERN/NULL_TERMINATED/KEY_VALUE_LIST have no length at all;
    # PIXEL_TRANSFER_SIZE/CLEAR_BUFFER_VALUE_COUNT compute a byte size from several siblings via
    # a runtime helper, not a single sibling's by-value count) - never comparable.
    return None


def find_pointer_length_mismatches(commands: dict[Api, list[Command]]) -> list[str]:
    """Cross-validate every EXISTING hand-authored POINTER_PARAM_OVERRIDES/
    ARRAY_OF_ARRAYS_PARAM_OVERRIDES entry's length-source against what gl.xml's own len=
    attribute would auto-derive for the same (command, param) - a pure lint pass, meant to be
    called once at codegen time (see generator.py) and its results printed as warnings, mirroring
    load_registries_and_extract_xml_data()'s own find_unknown_command_attributes/
    find_unknown_enum_attributes warnings. Returns a list of human-readable mismatch
    descriptions; an empty list means every hand entry this module can meaningfully compare
    agrees with gl.xml (true for every entry as of this module's introduction - see this
    module's own test suite, which pins that result down as a regression check).

    Never raises and never changes generated code - this is strictly informational. A hand entry
    is always authoritative regardless of what this function reports (see this module's header
    comment and pointer_metadata.py's own header comment on why direction/category can never be
    auto-derived at all - a "mismatch" here can only ever be about the length-SOURCE half of an
    entry, e.g. a hand entry pointing at the wrong sibling name, a stale length_multiplier after
    a gl.xml revision, or (for FIXED_COUNT) a hand literal that no longer matches gl.xml's own
    integer len=).

    `commands` covers all four APIs (gl/wgl/glx/egl) despite only gl.xml ever setting len= at
    all - POINTER_PARAM_OVERRIDES/ARRAY_OF_ARRAYS_PARAM_OVERRIDES key on plain command names with
    no API tag, and a handful of WGL entries use the raw, un-renamed registry name (e.g.
    'SetPixelFormat' rather than the interceptor-exported 'wglSetPixelFormat' - see
    pointer_metadata.py's own "Dual-registration" comment); this function looks up whichever
    (command_name, param_name) pair actually exists in `commands`, silently skipping an entry
    whose command_name isn't found at all (the renamed-only alias key, e.g.
    'wglSetPixelFormat', which the raw per-registry command list never contains under that
    name) rather than treating a naming convention this module has nothing to do with as a lint
    finding.
    """
    commands_by_name: dict[str, Command] = {}
    for api_commands in commands.values():
        for command in api_commands:
            # First match wins if a name were ever duplicated across APIs (does not happen
            # today - gl/wgl/glx/egl command names are already globally unique in practice,
            # verified by this not needing to pick between duplicates for any existing table
            # entry - see this module's own test suite).
            commands_by_name.setdefault(command.name, command)

    mismatches: list[str] = []

    for (command_name, param_name), override in POINTER_PARAM_OVERRIDES.items():
        hand = _hand_entry_as_length_source(
            override.length_source_kind, override.length_source_value, override.length_multiplier)
        if hand is None:
            continue  # Pure category kind - nothing length-source-shaped to compare.

        command = commands_by_name.get(command_name)
        if command is None:
            continue  # Renamed-only alias key (e.g. 'wglSetPixelFormat') - see this
                      # function's own docstring.
        param = next((p for p in command.params if p.name == param_name), None)
        if param is None:
            continue  # A genuine (command, param) mismatch is _validate_pointer_override's
                      # job (raises at generation time); this lint pass only ever compares
                      # length sources for pairs that structurally exist.

        other_params = [p for p in command.params if p is not param]
        auto = derive_length_source(param, other_params)
        if auto is None:
            continue  # gl.xml has no safe opinion here (COMPSIZE(...)/no len=/a divided-form
                      # length) - not a disagreement, just nothing to check the hand entry
                      # against. This is the common, expected case for entries that
                      # deliberately override a misleading COMPSIZE(...) hint (glGetShaderiv,
                      # glGetProgramiv, glGetFramebufferAttachmentParameteriv).

        auto_tuple = (auto.kind, auto.value, auto.multiplier)
        if hand != auto_tuple:
            mismatches.append(
                f'{command_name}.{param_name}: POINTER_PARAM_OVERRIDES has '
                f'length_source_kind={hand[0]}, length_source_value={hand[1]!r}, '
                f'length_multiplier={hand[2]}, but gl.xml len="{param.len}" derives '
                f'length_source_kind={auto.kind}, length_source_value={auto.value!r}, '
                f'length_multiplier={auto.multiplier}'
            )

    for (command_name, param_name), aoa_override in ARRAY_OF_ARRAYS_PARAM_OVERRIDES.items():
        command = commands_by_name.get(command_name)
        if command is None:
            continue
        param = next((p for p in command.params if p.name == param_name), None)
        if param is None:
            continue

        other_params = [p for p in command.params if p is not param]
        auto = derive_length_source(param, other_params)
        if auto is None:
            continue

        # ArrayOfArraysOverride has no length_multiplier field (every real array-of-arrays
        # occurrence today - glShaderSource's `string` - has a plain 1:1 outer count, never a
        # scaled one), so only the sibling name itself is comparable, not a multiplier.
        if auto.kind is not LengthSourceKind.SIBLING_PARAM or auto.multiplier != 1 or \
                auto.value != aoa_override.outer_count_param:
            mismatches.append(
                f'{command_name}.{param_name}: ARRAY_OF_ARRAYS_PARAM_OVERRIDES has '
                f'outer_count_param={aoa_override.outer_count_param!r}, but gl.xml '
                f'len="{param.len}" derives '
                f'(kind={auto.kind}, value={auto.value!r}, multiplier={auto.multiplier})'
            )

    return mismatches
