#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Semi-automatic auto-derivation for the "uniform setter" `value` parameter shared by
# glUniform*v/glProgramUniform*v (vector) and glUniformMatrix*v/glProgramUniformMatrix*v
# (matrix) - the sibling-FAMILY counterpart of pixel_transfer_metadata.py's
# derive_pixel_transfer_override(). Read that module's own header comment and
# pointer_length_derivation.py's header comment first; this one only explains what differs.
#
# WHY THIS IS SAFE DESPITE pointer_metadata.py's OWN "direction/category is never derivable
# from gl.xml alone" RULE: that rule is about a single (command, param) pair judged in
# isolation - gl.xml's len= truly carries no signal there (see
# pointer_length_derivation.py's glShaderSource.string vs
# glMultiDrawElementsBaseVertex.indices example: identical len=/shape, opposite dereference
# semantics). This module does not judge one pair in isolation. It only ever fires for a
# param named `value` on a command whose NAME matches one of two closed, exhaustively-checked
# regexes (_VECTOR_SETTER_RE / _MATRIX_SETTER_RE below) - i.e. it only ever extends a category
# ALREADY verified by a human for 20 other members of the exact same two command families
# (pointer_metadata.py's own POINTER_PARAM_OVERRIDES: glUniform{1,2,3,4}{f,i,ui}v,
# glUniform4dv, glUniformMatrix{2,3,4}fv, glUniformMatrix4dv, glProgramUniform4dv,
# glProgramUniformMatrix4dv, glProgramUniformMatrix4fv), to siblings distinguished from those
# 20 ONLY by the exact kind of variation the GL spec defines these two families over: vector
# width (1-4), scalar type (f/i/ui/d), matrix shape (square 2x2..4x4 and non-square
# 2x3/2x4/3x2/3x4/4x2/4x3), and DSA-vs-classic (glProgramUniform* vs glUniform*) - never a
# vendor-suffixed alias (ARB/EXT/NV/IMG - excluded by both regexes anchoring on `v$` with
# nothing after) and never the newer 64-bit-integer/bindless-handle families
# (glUniform*i64*v/glUniform*ui64*v/glUniform*Handleui64*v), which are a genuinely different
# value-type domain no existing hand entry has ever covered and this module deliberately does
# not extend to.
#
# Verified directly against the checked-out third_party/OpenGL-Registry/xml/gl.xml: every one
# of the 68 command names either regex matches (32 vector: glUniform/glProgramUniform x
# {1,2,3,4} x {f,i,ui,d}; 36 matrix: glUniformMatrix/glProgramUniformMatrix x
# {2,2x3,2x4,3,3x2,3x4,4,4x2,4x3} x {f,d}) has:
#   - the exact same param ROLES as the 20 already-audited siblings: (location, count, value)
#     / (program, location, count, value) for vector, (location, count, transpose, value) /
#     (program, location, count, transpose, value) for matrix - never an extra or missing
#     parameter that would suggest a different real signature;
#   - `value`'s own len= exactly "count" or "count*N" (never COMPSIZE(...) or anything else),
#     with N mechanically equal to the vector width or (rows*cols) the command's own NAME
#     already states - double-checked below by cross-referencing the regex-implied multiplier
#     against pointer_length_derivation.py's own independent len=-driven derivation, so a
#     hypothetical future gl.xml revision that changed one member's len= without also
#     "renaming" it can never silently produce a wrong multiplier (see
#     derive_uniform_setter_override()'s own multiplier cross-check).
# No exception found across all 68 - unlike pixel_transfer_metadata.py's own audit, which found
# several real excluded shapes (glColorSubTable, glClear*, SGIS 4-D), this family has none: the
# GL spec defines exactly one semantic for "set a uniform from a client-side array of
# vectors/matrices", with no OUT variant and no client-memory-vs-buffer-offset duality (unlike
# glDrawElements/glVertexAttribPointer's OPAQUE_BIT_PATTERN(_ARRAY) shapes) - `value` is always
# genuine client memory the driver copies FROM, never a buffer-relative offset the same bit
# pattern could otherwise mean.
#
# Wired into intermediates_creator.py's _is_param_capturable as a second FALLBACK (after
# pixel_transfer_metadata.py's own), consulted only when POINTER_PARAM_OVERRIDES has no
# explicit entry for the (command, param) pair - same "explicit table always wins" precedent
# every other fallback in this codebase follows. generator_helpers.py's pointer_override_for()
# (the Mako-facing lookup) is updated in lockstep, per pointer_length_derivation.py's own
# documented wiring contract.

import re

from intermediates import Parameter
from pointer_length_derivation import derive_length_source
from pointer_metadata import LengthSourceKind, PointerDirection, PointerParamOverride

# Anchored full-string matches (re.fullmatch) - a vendor suffix (ARB/EXT/NV/IMG/...) after the
# trailing `v` never matches either pattern, exactly like the 64-bit-integer/bindless-handle
# families (`i64`/`ui64`/`Handleui64` never match `[fiud]` for the type-suffix group either).
_VECTOR_SETTER_RE = re.compile(r'^gl(Program)?Uniform([1234])(f|i|ui|d)v$')
_MATRIX_SETTER_RE = re.compile(r'^gl(Program)?UniformMatrix([234])(?:x([234]))?(f|d)v$')


def _expected_multiplier(command_name: str) -> int | None:
    """The vector-width or (rows*cols) multiplier `command_name`'s own name states, or None if
    it matches neither family regex. Kept separate from derive_uniform_setter_override() so the
    "what the name says" and "what gl.xml's len= independently says" computations are visibly
    two different code paths, cross-checked rather than merged into one."""
    vector_match = _VECTOR_SETTER_RE.match(command_name)
    if vector_match is not None:
        return int(vector_match.group(2))
    matrix_match = _MATRIX_SETTER_RE.match(command_name)
    if matrix_match is not None:
        rows = int(matrix_match.group(2))
        cols = int(matrix_match.group(3)) if matrix_match.group(3) is not None else rows
        return rows * cols
    return None


def derive_uniform_setter_override(
    command_name: str, param: Parameter, other_params: list[Parameter],
) -> PointerParamOverride | None:
    """Auto-derive a complete SIBLING_PARAM PointerParamOverride for a glUniform*v/
    glProgramUniform*v/glUniformMatrix*v/glProgramUniformMatrix*v command's `value` parameter,
    or None if no safe derivation applies - None is always the safe answer; this function never
    guesses (see this module's header comment for exactly what evidence is required).

    `other_params` mirrors every other fallback/validator in this codebase's own convention
    (pixel_transfer_metadata.py's derive_pixel_transfer_override(),
    _validate_pointer_override()'s sibling checks) - needed to resolve and validate the `count`
    sibling gl.xml's len= names, and (for the matrix family) to confirm a `transpose` sibling
    is genuinely present, an extra structural safety net matching this family's own signature
    exactly (see pixel_transfer_metadata.py's own `format`/`type` <param group=...> check for
    the same "don't trust the name pattern alone" philosophy).
    """
    if param.name != 'value':
        return None

    expected_multiplier = _expected_multiplier(command_name)
    if expected_multiplier is None:
        return None

    if _MATRIX_SETTER_RE.match(command_name) is not None:
        transpose = next((p for p in other_params if p.name == 'transpose'), None)
        if transpose is None or not transpose.type_facts.is_scalar:
            return None

    location = next((p for p in other_params if p.name == 'location'), None)
    if location is None or not location.type_facts.is_scalar:
        return None

    derivation = derive_length_source(param, other_params)
    if derivation is None:
        return None
    if derivation.kind is not LengthSourceKind.SIBLING_PARAM:
        return None
    if derivation.value != 'count':
        return None
    if derivation.multiplier != expected_multiplier:
        # gl.xml's own len= disagrees with what the command's name says it should be - a
        # signal this member is not the plain, already-audited shape after all (e.g. a future
        # registry revision, or a name this regex matches by coincidence). Fail safe rather
        # than trusting either source alone.
        return None

    return PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.SIBLING_PARAM,
        length_source_value='count',
        length_multiplier=derivation.multiplier,
    )
