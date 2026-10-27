#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Semi-automatic auto-derivation for LengthSourceKind.PIXEL_TRANSFER_SIZE - the
# COMPSIZE(format,type,<dims...>) pixel-transfer counterpart of object_name_lifecycle.py's
# class=-based auto-derivation and pointer_length_derivation.py's len=-driven length-source
# auto-derivation. Read those two modules' own header comments first; this one only explains
# what differs.
#
# WHY THIS IS ITS OWN MODULE, NOT A pointer_length_derivation.py ADDITION: that module's own
# header comment is explicit that COMPSIZE(...) is NEVER trusted as a length by itself, for any
# command - "not a formula at all... never the count itself" - and correctly never wires a
# COMPSIZE-based fallback into intermediates_creator.py's _is_param_capturable gate. That
# constraint is still true of COMPSIZE(...) *in general* - a bare dependency list can name
# almost anything (see this module's own audit below: COMPSIZE(pname), COMPSIZE(target,level,
# format,type), COMPSIZE(buffer) all appear elsewhere in the registry, each needing a
# completely different real formula, some pname/target-dependent in ways this generator has no
# safe generic answer for at all - see pointer_metadata.py's TEX_SAMPLER_PARAMETER_VALUE_COUNT/
# PATCH_PARAMETER_VALUE_COUNT/CLEAR_BUFFER_VALUE_COUNT, each its own small hand-written runtime
# helper, never a shared "COMPSIZE means X" rule).
#
# What IS safe to auto-derive is one single, narrow, exhaustively-audited COMPSIZE shape: a
# pixel-transfer buffer whose COMPSIZE(...) argument list is *exactly*
# (format, type, width[, height[, depth]]) - i.e. the same four/five sibling names
# ComputePixelTransferByteSize() (OpenGL/common/layer_interface/pixelTransferSize.h) already
# takes as parameters, present in exactly that order, naming siblings that are themselves
# plain scalar params with the expected gl.xml <param group="..."> annotations (PixelFormat/
# PixelType) that confirm `format`/`type` really are pixel-transfer format/type enums, not some
# unrelated same-named parameter on a different kind of command. Audited exhaustively against
# every COMPSIZE(...) occurrence in the checked-out third_party/OpenGL-Registry/xml/gl.xml
# whose argument list starts with literal `format, type` (44 occurrences across gl/glX EXT/OES/
# SGIS variants of Tex/TexSubImage/DrawPixels/ColorTable/ConvolutionFilter/MultiTexImage*EXT/
# TextureImage*EXT):
#   - Every single one uses this exact (format, type, width[, height[, depth]]) shape once you
#     restrict to "starts with format, type AND the remaining names are width/height/depth in
#     that order" - EXCEPT glColorSubTable (COMPSIZE(format,type,count) - a 1-D update whose
#     third name is `count`, not `width`), glClearBufferData/glClearBufferSubData/
#     glClearNamedBufferData(EXT)/glClearNamedBufferSubData(EXT)/glClearTexImage(EXT)/
#     glClearTexSubImage(EXT) (COMPSIZE(format,type) - a single clear-color PATTERN value with
#     NO dimension arguments at all, not an image; derived separately as TEXEL_BYTE_SIZE, see
#     _derive_texel_byte_size_override), and glTexImage4DSGIS/glTexSubImage4DSGIS
#     (COMPSIZE(format,type,width,height,depth,size4d) - a vendor-only 4-D texture extension
#     with a genuine fifth dimension no version of ComputePixelTransferByteSize() computes).
#     Every one of these is real, deliberately excluded evidence that "COMPSIZE(format,type,
#     ...)" is not by itself sufficient signal - the trailing dimension-name LIST itself must
#     be checked against the literal, finite set of shapes ComputePixelTransferByteSize() (and
#     the plain-1D case, which passes height=1) actually implements: (width,),
#     (width, height), (width, height, depth) - see _DIMENSION_SHAPES below. Nothing here
#     "generically" accepts an arbitrary trailing name list; every accepted shape is one this
#     module's own runtime counterpart genuinely knows how to compute, and every rejected shape
#     above is either a different formula (glColorSubTable/glClear*) or one this backend does
#     not implement at all (SGIS 4-D) rather than one this module happens not to recognize yet.
#   - `format`/`type` carry <param group="PixelFormat">/<param group="PixelType"> on every one
#     of the matches this module accepts (verified against the same audit) - checked below as
#     an extra, load-bearing safety net (not merely documentation): it is what actually
#     distinguishes a genuine pixel-transfer command's `format`/`type` pair from a
#     same-COMPSIZE-shape-by-coincidence, different-meaning `format`/`type` pair a future
#     registry revision might introduce under some other group.
#   - `width`/`height`/`depth` are always plain GLsizei scalars (TypeFacts.is_scalar) on every
#     match - checked below too, mirroring _validate_pointer_override's own sibling-is-scalar
#     checks for every other LengthSourceKind.
#
# DIRECTION (IN vs OUT): pointer_metadata.py's own header comment is unambiguous that direction
# can never be derived from gl.xml alone, IN GENERAL. This module does not contradict that -
# it relies on a narrower, separately-verified fact specific to this one category: every
# COMPSIZE(format,type,<dims>)-shaped pixel-transfer pointer in the audit above is declared
# EITHER `const void *` (glTexImage2D/glTexSubImage3D/glDrawPixels/glMultiTexImage2DEXT/...,
# all IN - the driver reads application-owned pixel data) OR plain `void *` (glReadPixels, the
# only OUT example in the audit), never anything else - there is no third spelling and no
# command where the same pointer text means the opposite direction. This is therefore reading
# an actual, exhaustively-checked structural fact (the pointer's own const-qualification text)
# for this one narrow category, not inferring direction from "looks like an output" guesswork -
# see derive_pixel_transfer_override()'s own direction check below, which requires the
# parameter's base type to be exactly 'void' (not just any is_array_element_eligible type) and
# its declared type text to be exactly 'const void *' or 'void *', falling back to None (no
# derivation at all) for anything else rather than guessing.
#
# Wired into intermediates_creator.py's _is_param_capturable as a FALLBACK, consulted only when
# POINTER_PARAM_OVERRIDES has no explicit entry for the (command, param) pair - mirroring
# object_name_lifecycle.py's _EXPLICIT_OVERRIDES precedent and pointer_length_derivation.py's
# own documented wiring contract for derive_pointer_param_override(): the explicit table always
# wins, so a command whose real semantics genuinely differ from the plain
# ComputePixelTransferByteSize() formula (a 1D-vs-2D-vs-3D-vs-array-vs-cube variant needing a
# different helper or call shape) can still add a manual POINTER_PARAM_OVERRIDES entry that
# silently takes priority over this fallback - this module never overrides a hand-authored
# entry, it only ever fills in commands that had none.

from intermediates import Parameter
from pointer_metadata import LengthSourceKind, PointerDirection, PointerParamOverride

# The only trailing-dimension-name shapes (after the mandatory leading `format, type`) this
# module ever accepts - see this module's own header comment for the exhaustive audit that
# arrived at exactly these three, and no others (in particular: never `()` - a same-value
# clear-color pattern, not an image; never anything containing `count`/`size4d` - a different
# real formula/dimensionality entirely).
_DIMENSION_SHAPES: frozenset[tuple[str, ...]] = frozenset({
    ('width',),
    ('width', 'height'),
    ('width', 'height', 'depth'),
})


def _parse_compsize_args(len_attrib: str | None) -> tuple[str, ...] | None:
    """Return COMPSIZE(...)'s comma-separated argument names, or None if `len_attrib` is not a
    COMPSIZE(...) expression at all (mirrors pointer_length_derivation.py's own
    classify_len_attrib COMPSIZE detection, kept local/minimal here rather than importing that
    module's private regex - this module only ever needs to recognize the shape, not the full
    LenAttribShape taxonomy that module classifies every len= value into)."""
    if len_attrib is None or not (len_attrib.startswith('COMPSIZE(') and len_attrib.endswith(')')):
        return None
    inner = len_attrib[len('COMPSIZE('):-1]
    return tuple(name.strip() for name in inner.split(','))


def _derive_texel_byte_size_override(
    param: Parameter, by_name: dict[str, Parameter],
) -> PointerParamOverride | None:
    """COMPSIZE(format,type) - a single clear value (glClearTexImage/glClearBufferData/...), see
    LengthSourceKind.TEXEL_BYTE_SIZE."""
    format_param = by_name.get('format')
    type_param = by_name.get('type')
    if format_param is None or type_param is None:
        return None
    if format_param.group != 'PixelFormat' or type_param.group != 'PixelType':
        return None
    if not format_param.type_facts.is_scalar or not type_param.type_facts.is_scalar:
        return None
    if param.type != 'const void *':
        return None
    return PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.TEXEL_BYTE_SIZE,
        pixel_format_param='format',
        pixel_type_param='type',
    )


def derive_pixel_transfer_override(
    param: Parameter, other_params: list[Parameter],
) -> PointerParamOverride | None:
    """Auto-derive a complete PIXEL_TRANSFER_SIZE PointerParamOverride for `param`, or None if
    no safe derivation applies - None is always the safe answer; this function never guesses
    (see this module's header comment for exactly what evidence is required before it returns
    non-None).

    `other_params` is every other parameter on the same command (mirrors
    _validate_pointer_override's/pointer_length_derivation.py's own `other_params` convention) -
    needed to resolve and validate the format/type/dimension siblings COMPSIZE(...) names.
    """
    by_name = {p.name: p for p in other_params}
    compsize_args = _parse_compsize_args(param.len)
    if compsize_args is None and param.len is None and param.type == 'const void *':
        # gl.xml omits len= on core glTextureSubImage1D/2D/3D's `pixels` (their EXT
        # counterparts carry COMPSIZE(format,type,<dims>)). The same shape is recovered from the
        # siblings instead; the format/type group checks below still apply. IN only - the one
        # OUT match (glExtGetTexSubImageQCOM) stays excluded.
        compsize_args = ('format', 'type') + tuple(
            name for name in ('width', 'height', 'depth') if name in by_name)
    if compsize_args == ('format', 'type'):
        return _derive_texel_byte_size_override(param, by_name)
    if compsize_args is None or len(compsize_args) < 3:
        return None
    if compsize_args[0] != 'format' or compsize_args[1] != 'type':
        return None
    dims = compsize_args[2:]
    if dims not in _DIMENSION_SHAPES:
        return None

    format_param = by_name.get('format')
    type_param = by_name.get('type')
    if format_param is None or type_param is None:
        return None
    if format_param.group != 'PixelFormat' or type_param.group != 'PixelType':
        return None
    if not format_param.type_facts.is_scalar or not type_param.type_facts.is_scalar:
        return None

    dim_params = [by_name.get(name) for name in dims]
    if any(p is None or not p.type_facts.is_scalar for p in dim_params):
        return None

    # Direction: an exhaustively-checked structural fact for this one narrow category, not a
    # general inference - see this module's header comment. Requires the base type to be
    # exactly 'void' (every real match in the audit is a byte-blob pixel buffer, never e.g. a
    # single-indirection GLfloat* that happens to share some other COMPSIZE shape) and the full
    # declared type text to be exactly one of the two spellings every match in the audit uses.
    if param.type_facts.base_type != 'void' or param.type_facts.pointer_depth != 1:
        return None
    if param.type == 'const void *':
        direction = PointerDirection.IN
    elif param.type == 'void *':
        direction = PointerDirection.OUT
    else:
        return None

    return PointerParamOverride(
        direction=direction,
        length_source_kind=LengthSourceKind.PIXEL_TRANSFER_SIZE,
        pixel_format_param='format',
        pixel_type_param='type',
        pixel_width_param=dims[0],
        pixel_height_param=dims[1] if len(dims) >= 2 else None,
        pixel_depth_param=dims[2] if len(dims) >= 3 else None,
    )
