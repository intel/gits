#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Semi-automatic auto-derivation for LengthSourceKind.TEX_SAMPLER_PARAMETER_VALUE_COUNT - the
# glTexParameter*v/glSamplerParameter*v sibling-FAMILY counterpart of uniform_setter_metadata.py
# (a NAME-driven sibling-family generalization, not a len=-driven one like
# pointer_length_derivation.py, and not a COMPSIZE(format,type,<dims>)-shape one like
# pixel_transfer_metadata.py). Read those three modules' own header comments first; this one
# only explains what differs.
#
# WHY THIS IS SAFE DESPITE COMPSIZE(...) NEVER BEING A TRUSTWORTHY LENGTH BY ITSELF (see
# pointer_length_derivation.py's own header comment and pixel_transfer_metadata.py's "why this
# is its own module" section): this module does not try to compute a length from COMPSIZE(pname)
# generically. It only ever extends a category ALREADY verified by a human for 3 hand-authored
# POINTER_PARAM_OVERRIDES entries - glTexParameterfv, glTexParameteriv, glSamplerParameterfv -
# whose `params`/`param` element count is 4 for GL_TEXTURE_BORDER_COLOR (every form, both
# families) and 4 for GL_TEXTURE_SWIZZLE_RGBA on the Tex- family ONLY (never the Sampler-
# family - see TexOrSamplerParameterFamily's comment in
# pointer_metadata.py for why), 1 for every other legal `pname`, computed at capture time by a
# real runtime helper (gits::opengl2::ComputeTexOrSamplerParameterValueCount(),
# OpenGL2/common/layer_interface/pixelTransferSize.h) that switches on the `pname` VALUE AND an
# explicit TexOrSamplerParameterCommand family discriminator this module now supplies via
# derive_tex_sampler_parameter_override()'s own `tex_sampler_family` field below - it is NOT
# "caller-agnostic" for GL_TEXTURE_SWIZZLE_RGBA (an earlier version of this module and this
# comment incorrectly assumed it was, which was itself an out-of-bounds-read hazard: a real app
# calling glSamplerParameterIiv(sampler, GL_TEXTURE_SWIZZLE_RGBA, ...) would have made a
# family-agnostic helper read 4 elements from a 1-element caller-supplied buffer, before the
# driver's own GL_INVALID_ENUM rejection of the malformed call ever had a chance to matter).
# Extending which
# command names are allowed to reach that helper is therefore safe only because this module
# also tells the helper which family each one belongs to - never because the helper's answer
# was already family-independent.
#
# Exhaustively verified directly against the checked-out
# third_party/OpenGL-Registry/xml/gl.xml: fully-anchored regex `^gl(Tex|Sampler)Parameter(I)?
# (f|i|ui)v$` matches exactly 8 command names - glTexParameterfv, glTexParameteriv,
# glTexParameterIiv, glTexParameterIuiv, glSamplerParameterfv, glSamplerParameteriv,
# glSamplerParameterIiv, glSamplerParameterIuiv - and every one of them:
#   - has the exact same (target|sampler, pname, params|params|param) signature as the 3
#     already-audited siblings - `params` (plural) for every Tex- command, `param` (singular)
#     for every Sampler- command, never an extra or missing parameter;
#   - carries len="COMPSIZE(pname)" verbatim - the identical raw string as the 3 already-audited
#     siblings, not merely a similar-looking one - checked explicitly below rather than assumed
#     from the regex match alone;
#   - has a `pname` sibling that is a plain scalar GLenum, exactly like the 3 already-audited
#     siblings' own `pname`.
# No exception found: the "I" (glTexParameterIiv/Iuiv, glSamplerParameterIiv/Iuiv - integer
# texture/sampler parameters, used to set GL_TEXTURE_BORDER_COLOR etc. as raw integers instead
# of normalized floats) and non-"I" forms differ only in HOW the border-color/swizzle value is
# interpreted by the driver, never in HOW MANY elements it occupies WITHIN THE SAME FAMILY - but
# the two FAMILIES themselves are not interchangeable for swizzle: gl.xml gives
# glTexParameterfv/iv/Iiv/Iuiv's own `pname` param group="TextureParameterName" (identical
# across all four Tex- forms, "I" forms included - so GL_TEXTURE_SWIZZLE_RGBA, itself tagged
# group="TextureParameterName", is legal for all four), but glSamplerParameterfv/iv/Iiv/Iuiv's
# own `pname` param carries group="SamplerParameterF" (fv) or group="SamplerParameterI"
# (iv/Iiv/Iuiv) instead - neither Sampler group's own enum membership in gl.xml includes
# GL_TEXTURE_SWIZZLE_RGBA/R/G/B/A anywhere in the registry (texture swizzle is per-texture-
# OBJECT state; samplers have no swizzle state at all to set, "I" forms included). Every one of
# the 5 new siblings is therefore correctly handled by passing this module's own `command_name`
# match (Tex- vs Sampler-, from `match.group(1)` below) through as the family discriminator -
# NOT by the pname-value switch alone.
#
# pointer_length_derivation.py's len=-driven cross-check machinery deliberately does not apply
# here: COMPSIZE(...) is exactly the one len= shape that module's own classify_len_attrib()
# never trusts as a length by itself (see its own header comment) - there is no numeric
# multiplier to cross-check the way uniform_setter_metadata.py cross-checks `count*N` against a
# name-implied N. The safety argument here is the narrower, name-anchored one above: an
# EXHAUSTIVE match against every literal "COMPSIZE(pname)"-shaped sibling in the family, not a
# len=-derived computation.
#
# NOT extended to (each excluded for a specific, checked reason, not merely unaudited):
#   - The OUT-direction getters (glGetTexParameterfv/iv/Iiv/Iuiv, glGetSamplerParameterfv/iv/
#     Iiv/Iuiv) share the identical COMPSIZE(pname) shape and the identical safe-by-pname-value
#     count - but intermediates_creator.py's _validate_pointer_override raises for any
#     TEX_SAMPLER_PARAMETER_VALUE_COUNT override that isn't PointerDirection.IN, because
#     commandsAuto.h.mako/recorderWrappersAuto.cpp.mako only ever emit an IN-direction call to
#     ComputeTexOrSamplerParameterValueCount() - there is no OUT-direction Mako branch to emit
#     into. Enabling these would need new template/runtime work, not just a metadata-table
#     fallback, so this module deliberately never derives a non-IN override and never matches
#     a `glGet...` command name at all.
#   - glTexParameterxv/glTexParameterxvOES (GLES/OES fixed-point `GLfixed` variants) share the
#     COMPSIZE(pname) shape but were not part of the audit that introduced
#     ComputeTexOrSamplerParameterValueCount() - excluded by this module's own regex (the
#     type-suffix group is `(f|i|ui)`, never `x`) pending a real check of whether GLES's fixed-
#     point pname set is actually identical, rather than assumed.
#   - glTextureParameterfv/iv/Iiv/Iuiv/glGetTextureParameter*v/glMultiTexParameter*vEXT/
#     glGetMultiTexParameter*vEXT (the 4.5 direct-state-access and pre-DSA multi-bind
#     `EXT`/`Texture`-prefixed variants) never match this module's regex at all - the anchor
#     requires "Tex" or "Sampler" immediately followed by "Parameter", which "Texture"/"Multi
#     Tex" both break structurally, not merely by vendor-suffix convention.
#   - glPatchParameterfv (a different COMPSIZE(pname) family entirely - 4 for
#     GL_PATCH_DEFAULT_OUTER_LEVEL, 2 for GL_PATCH_DEFAULT_INNER_LEVEL, computed by its own
#     ComputePatchParameterValueCount() helper) has no sibling family at all: GL defines no
#     glPatchParameteriv counterpart, so there is nothing to generalize to.
#
# Wired into intermediates_creator.py's _is_param_capturable as a third FALLBACK (after
# pixel_transfer_metadata.py's and uniform_setter_metadata.py's own), consulted only when
# POINTER_PARAM_OVERRIDES has no explicit entry for the (command, param) pair - same
# "explicit table always wins" precedent every other fallback in this codebase follows.
# generator_helpers.py's pointer_override_for() (the Mako-facing lookup) is updated in lockstep.

import re

from intermediates import Parameter
from pointer_metadata import (
    LengthSourceKind,
    PointerDirection,
    PointerParamOverride,
    TexOrSamplerParameterFamily,
)

# Anchored full-string match (re.fullmatch) - "Texture"/"Multi Tex"-prefixed commands and any
# vendor/GLES suffix (EXT/OES/ARB/...) never match, exactly like uniform_setter_metadata.py's
# own vector/matrix regexes anchor on `v$` with nothing after.
_TEX_SAMPLER_PARAMETER_RE = re.compile(r'^gl(Tex|Sampler)Parameter(I)?(f|i|ui)v$')

# The exact len= string every already-audited and newly-derived sibling carries - checked
# verbatim, not merely inferred from the regex match, mirroring
# pixel_transfer_metadata.py's own _parse_compsize_args() shape check.
_EXPECTED_LEN = 'COMPSIZE(pname)'


def derive_tex_sampler_parameter_override(
    command_name: str, param: Parameter, other_params: list[Parameter],
) -> PointerParamOverride | None:
    """Auto-derive a complete IN-direction TEX_SAMPLER_PARAMETER_VALUE_COUNT
    PointerParamOverride for a glTexParameter{,I}{i,ui}v/glTexParameterfv/
    glSamplerParameter{,I}{i,ui}v/glSamplerParameterfv command's `params`/`param` parameter, or
    None if no safe derivation applies - None is always the safe answer; this function never
    guesses (see this module's header comment for exactly what evidence is required).

    `other_params` mirrors every other fallback/validator in this codebase's own convention -
    needed to resolve and validate the `pname` sibling ComputeTexOrSamplerParameterValueCount()
    needs.
    """
    match = _TEX_SAMPLER_PARAMETER_RE.match(command_name)
    if match is None:
        return None

    expected_param_name = 'params' if match.group(1) == 'Tex' else 'param'
    if param.name != expected_param_name:
        return None
    if param.len != _EXPECTED_LEN:
        # gl.xml disagrees with the shape every already-audited sibling carries - not a
        # member of this family after all (e.g. a future registry revision, or a name this
        # regex matches by coincidence). Fail safe rather than trusting the name alone.
        return None

    pname = next((p for p in other_params if p.name == 'pname'), None)
    if pname is None or not pname.type_facts.is_scalar:
        return None

    # Family discriminator - never inferred at the C++/Mako call site, always
    # supplied here from the same regex match that already distinguished 'params' vs 'param'
    # above, so there is exactly one place in this module that decides Tex- vs Sampler-.
    family = (TexOrSamplerParameterFamily.TEX_PARAMETER if match.group(1) == 'Tex'
              else TexOrSamplerParameterFamily.SAMPLER_PARAMETER)

    return PointerParamOverride(
        direction=PointerDirection.IN,
        length_source_kind=LengthSourceKind.TEX_SAMPLER_PARAMETER_VALUE_COUNT,
        pname_param='pname',
        tex_sampler_family=family,
    )
