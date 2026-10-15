#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Classifies GL object-name (GLuint) fields into (GLObjectKind, ObjectNameRole) - which
# namespace a field belongs to, and whether it creates/looks-up/deletes a name in that
# namespace.
#
# HISTORY / CORRECTED PREMISE: earlier revisions of this module claimed a GLuint field's
# namespace "can never be inferred from its type alone" (true - unlike HGLRC, or unlike
# Vulkan's VkImage/VkBuffer/etc., every GL object-name namespace shares the exact same C++
# type, GLuint) and concluded from that alone that this had to be a fully hand-maintained
# table, entry by entry, forever. That conclusion was wrong: gl.xml's own <param class="..."/>
# / <proto class="..."/> annotation (parsed into Parameter.class_/ReturnValue.class_ by
# intermediates_creator.py's normalize_attribs; already used by that module's
# _requires_object_name_override as a presence check) does not merely mark a field as *some*
# object name - the ATTRIBUTE'S OWN STRING VALUE names its exact namespace: `class="texture"`,
# `class="buffer"`, `class="vertex array"`, `class="program pipeline"`, and so on - the same
# eleven values GLObjectKind's own enumerators mirror one-for-one (`str.title().replace(' ',
# '')`, e.g. "vertex array" -> "VertexArray", "program pipeline" -> "ProgramPipeline"). Found
# concretely while investigating why B.1's systemic gate silently disabled glFramebufferTexture
# (see modern-opengl-backend-architecture.md's B.1 incident writeup): the analysis that
# prompted this rewrite pointed out that Vulkan's vk.xml makes handle *kind* free (every handle
# is its own C++ type - see Vulkan/codegen/intermediates_creator.py's parse_handles/
# handle_names), and asked, correctly, whether gl.xml has an equivalent this module was simply
# not using yet. It does, for `class=`; auditing every gl.xml command confirmed this (see
# docs/development/modern-opengl-backend-architecture.md's auto-derivation decision for the
# full audit results) - zero disagreements between the old 80-entry hand table and (class_,
# naming) auto-derivation across every field the old table covered.
#
# ROLE (CREATE/LOOKUP/DELETE) still comes from the command's own NAME, not from `class=` (which
# only ever says *which* namespace, never *what this specific call does* to it) - but that,
# too, is a reliable, auditable pattern across the whole registry, not a per-command judgment
# call: `glGen*`/`glCreate*` (as a whole next CamelCase word, not merely a substring - see
# _GEN_OR_CREATE_RE's own comment for why `glGenerateMipmap`/`glGenerateTextureMipmap` are NOT
# false positives) creates; `glDelete*` deletes; everything else (including every `glIs*`
# query, every `glBind*`, every DSA "named object" accessor) looks up an existing name. This
# was verified, not assumed: every one of the ~700 class-tagged fields with no prior manual
# entry was audited by leading verb (see the auto-derivation decision's own audit log) - the
# ONLY genuine exception found across the entire registry is `glFreeObjectBufferATI` (below),
# an obsolete/deprecated ATI_map_object_buffer-only entry point whose name does not match
# `glDelete*` despite actually deleting its buffer's name.
#
# `resolve_object_name_override()` below is now the single source of truth
# intermediates_creator.py / generator_helpers.py call into (replacing direct
# OBJECT_NAME_PARAM_OVERRIDES.get() lookups): it auto-derives from (class_, command_name) and
# consults _EXPLICIT_OVERRIDES only for a documented, individually-justified exception the
# metadata itself cannot resolve (see _EXPLICIT_OVERRIDES's own header comment) - the
# systemic-gate parallel to how B.1 itself already made "no override -> stays disabled" the
# structural default, just now with almost nothing left that needs a manual override at all.

import re
from dataclasses import dataclass

try:
    from enum import StrEnum
except ImportError:
    from enum import Enum
    class StrEnum(str, Enum):
        def __str__(self) -> str:
            return self.value


class GLObjectKind(StrEnum):
    """Which GL object-name namespace a GLuint field belongs to.

    Values are the exact identifier text used for the corresponding C++
    `gits::opengl2::GLObjectKind` enumerator (see common/services/objectNameMapService.h) -
    generator_helpers.py/the recorder and player Mako templates print `.value` directly into
    generated C++ as `GLObjectKind::<value>`. Also the auto-derivation target of
    _CLASS_TO_KIND below: every enumerator name here is gl.xml's own `class="..."` string,
    title-cased with spaces removed (e.g. "vertex array" -> VertexArray, "program pipeline" ->
    ProgramPipeline) - a new enumerator is only ever added once a real, tested caller needs the
    backend's ObjectNameMapService support for that namespace (see each value's own history
    note below); _CLASS_TO_KIND's entry for that same gl.xml class string is added in the same
    change, never speculatively ahead of it.

    FRAMEBUFFER/VERTEX_ARRAY do have entries below now (see the FBO-pipeline round,
    docs/development/opengl2-player-skeleton-plan.md's Decision 10) - both kinds were declared
    here (and in the C++ enum) ahead of their first real use precisely so the bucketed
    service's shape would not need to change again the moment a Gen command for either was
    enabled. (RENDERBUFFER followed the same path later - see glFramebufferRenderbuffer's
    LOOKUP once it gained a real, already-`enabled` caller.)

    QUERY was added for glGenQueries/glDeleteQueries (see modern-opengl-backend-architecture.md's
    "Reduce replay warning noise" decision) - both are exercised by every tested application (the
    g-truc ogl-samples framework's `test` base class creates one timer query per window and
    deletes it on shutdown).
    """
    TEXTURE = 'Texture'
    BUFFER = 'Buffer'
    PROGRAM = 'Program'
    SHADER = 'Shader'
    FRAMEBUFFER = 'Framebuffer'
    VERTEX_ARRAY = 'VertexArray'
    QUERY = 'Query'
    # glBindRenderbuffer/glBindSampler/
    # glBindProgramPipeline were already `enabled=True` (scalar-only) with zero lifecycle
    # tracking for their own object namespace - the same latent gap glBindBufferBase/
    # glBindFragDataLocation/... had before earlier rounds' own audits fixed those.
    RENDERBUFFER = 'Renderbuffer'
    SAMPLER = 'Sampler'
    # Per the GL spec's "Shared Objects and Multiple Contexts" rule (see
    # shareGroupService.h's own header comment), program pipeline objects are, like
    # FRAMEBUFFER/VERTEX_ARRAY, container objects never shared between contexts - unlike
    # RENDERBUFFER/SAMPLER above, which (per that same rule) genuinely are shared. See
    # recorder/include/handleArgumentUpdaters.h's GroupIdentityFor(), which buckets this kind
    # by per-context identity for exactly this reason.
    PROGRAM_PIPELINE = 'ProgramPipeline'


class ObjectNameRole(StrEnum):
    """What a field with a resolved object-name override does with its name(s).

    Named CREATE/DELETE (not the earlier MINT/ERASE) to match GL's own vocabulary for these
    same operations (glCreateProgram/glCreateShader, glDeleteTextures/glDeleteProgram/
    glDeleteShader/...) instead of container-operation terminology (std::set::erase, "minting"
    a token/coin) that reads as a GITS-internal detail unrelated to what the GL call itself
    does.
    """

    CREATE = 'create'  # a newly-created name (or names) this call creates - glGenTextures'
                        # textures / glGenBuffers' buffers / glCreateProgram's and
                        # glCreateShader's return value.
    LOOKUP = 'lookup'   # a read-only reference to an existing, already-registered name -
                        # glBindTexture/glBindBuffer/glUseProgram/glBindAttribLocation/
                        # glGetUniformLocation's object-name parameter.
    DELETE = 'delete'   # an existing name (or names) destroyed on success, looked up first -
                        # glDeleteTextures/glDeleteProgram/glDeleteShader.


@dataclass(frozen=True, kw_only=True)
class ObjectNameOverride:
    kind: GLObjectKind
    role: ObjectNameRole


# gl.xml's own `class="..."` string -> GLObjectKind, or None for a namespace gl.xml itself
# already distinguishes but this backend has no ObjectNameMapService/GLObjectKind support for
# yet (sync objects, transform feedback objects, display lists, INTEL performance query
# handles/ids - see modern-opengl-backend-architecture.md's deferred-scope section). A field
# whose class_ maps to None here stays exactly as unresolvable as a field with no class_ at
# all - resolve_object_name_override() returns None either way, so intermediates_creator.py's
# _requires_object_name_override gate still leaves it (and its command, unless every other
# field independently qualifies) disabled, same as before this module existed.
#
# This dict is the ENTIRE kind side of auto-derivation - verified exhaustive against every
# class= value gl.xml actually uses today (see this module's header comment): if the registry
# is ever regenerated from a newer gl.xml revision that introduces a class= string not listed
# here, resolve_object_name_override() raises immediately (fail loud, not silently disabled) -
# see that function's own KeyError handling below, mirroring intermediates_creator.py's
# find_unknown_enum_attributes/find_unknown_command_attributes precedent for exactly this kind
# of "the registry grew a new thing we've never classified" situation.
_CLASS_TO_KIND: dict[str, GLObjectKind | None] = {
    'texture': GLObjectKind.TEXTURE,
    'buffer': GLObjectKind.BUFFER,
    'program': GLObjectKind.PROGRAM,
    'shader': GLObjectKind.SHADER,
    'framebuffer': GLObjectKind.FRAMEBUFFER,
    'vertex array': GLObjectKind.VERTEX_ARRAY,
    'query': GLObjectKind.QUERY,
    'renderbuffer': GLObjectKind.RENDERBUFFER,
    'sampler': GLObjectKind.SAMPLER,
    'program pipeline': GLObjectKind.PROGRAM_PIPELINE,
    # No GLObjectKind/ObjectNameMapService support yet - no tested sample needs any of these
    # namespaces (see modern-opengl-backend-architecture.md's deferred-scope section for why
    # each stays deferred). Listed explicitly (not just absent) so a field tagged with one of
    # these is a *documented*, deliberate "not yet" rather than indistinguishable from a
    # gl.xml class= value nobody has ever looked at.
    'sync': None,
    'transform feedback': None,
    'display list': None,
    'perf query handle': None,
    'perf query id': None,
}

# `glGen*`/`glCreate*` as a whole next CamelCase word (immediately followed by either the end
# of the name or an uppercase letter) creates a name; `glDelete*` (same rule) deletes one;
# everything else looks one up. The lookahead is load-bearing, not decorative:
# `glGenerateMipmap`/`glGenerateTextureMipmap`/`glGenerateMultiTexMipmapEXT` (regenerate mipmap
# levels of an EXISTING texture - a LOOKUP, not a CREATE) all start with the literal substring
# "glGen" but continue with a lowercase letter ("erate...", not a new capitalized word), so
# `^glGen(?:$|[A-Z])` correctly rejects them while still matching `glGenTextures`,
# `glGenBuffers`, bare `glGenLists`, etc. Verified against every class-tagged command in the
# registry with this exact leading verb (see the auto-derivation decision's own audit log) -
# no other Gen*/Create*/Delete*-prefixed command was found to misclassify this way.
_GEN_OR_CREATE_RE = re.compile(r'^gl(?:Gen|Create)(?:$|[A-Z])')
_DELETE_RE = re.compile(r'^glDelete(?:$|[A-Z])')


def _infer_role(command_name: str) -> ObjectNameRole:
    """Naming-pattern role heuristic - see _GEN_OR_CREATE_RE's own comment for why this is safe
    across the whole registry, not just the commands this module already knew about."""
    if _GEN_OR_CREATE_RE.match(command_name):
        return ObjectNameRole.CREATE
    if _DELETE_RE.match(command_name):
        return ObjectNameRole.DELETE
    return ObjectNameRole.LOOKUP


# Explicit, individually-justified exceptions the (class_, command-name) auto-derivation above
# cannot or should not resolve on its own. Checked FIRST by resolve_object_name_override(),
# before auto-derivation runs at all - so an entry here always wins, whether it is correcting a
# wrong auto-derived answer or (in principle) documenting a namespace/role combination the
# heuristic would refuse to derive.
#
# Deliberately kept as close to empty as the actual registry allows: every one of the ~80
# (command, field) pairs this table used to hand-list before this rewrite agreed EXACTLY with
# (class_, naming) auto-derivation (0 disagreements across the full audit - see
# modern-opengl-backend-architecture.md's auto-derivation decision) and was removed as pure
# redundancy, not because it became wrong. Do not re-add an entry here "for documentation" or
# "to be safe" if auto-derivation already produces the identical (kind, role) - that is exactly
# the kind of entry this rewrite exists to eliminate (a manual fact that can silently drift out
# of sync with the registry, like the glFramebufferTexture gap that prompted this whole
# investigation). Only add an entry when auto-derivation is DEMONSTRABLY wrong for that
# specific (command, field) pair, with a comment explaining exactly why - like the one entry
# below.
_EXPLICIT_OVERRIDES: dict[tuple[str, str], ObjectNameOverride] = {
    # ATI_map_object_buffer (2004-era, long obsolete, no modern driver exposes it) names its
    # buffer-releasing entry point glFreeObjectBufferATI - "Free", not "Delete" - despite it
    # doing exactly what glDeleteBuffers does for the one buffer name it takes: releasing that
    # name back to the driver (see the extension spec's own "the name buffer is deleted"
    # wording). _DELETE_RE only matches `glDelete*`, so naming-based auto-derivation would
    # otherwise call this a LOOKUP - wrong, and the only genuine role mismatch found across the
    # entire registry audit backing this module's rewrite (every other Free*/New*/Request*/
    # Prioritize*/Make*Resident*/Multicast*/Signal*/Wait*-prefixed class-tagged command
    # genuinely is a LOOKUP of an existing name - see that audit log for the full list checked).
    ('glFreeObjectBufferATI', 'buffer'): ObjectNameOverride(
        kind=GLObjectKind.BUFFER, role=ObjectNameRole.DELETE),
}


def resolve_object_name_override(
    command_name: str, field_name: str, class_: str | None,
) -> ObjectNameOverride | None:
    """The single source of truth for "does this (command, field) have an object-name
    classification, and if so what is it" - replaces every direct OBJECT_NAME_PARAM_OVERRIDES
    dict lookup this module used to expose (see this module's header comment for why a lookup
    table is no longer the primary mechanism).

    `class_` is the field's own Parameter.class_/ReturnValue.class_ (gl.xml's `class="..."`
    annotation, or None if gl.xml never tagged this field at all - a plain, non-object-name
    GLuint like a texture unit index or an enum-typed identifier, see e.g. glCopyImageSubData's
    srcName/glObjectLabel's name, which the registry itself deliberately leaves untagged
    because their real namespace is only known at runtime from a sibling GLenum). Returns None
    for exactly the same two cases _requires_object_name_override's caller needs to distinguish
    from a real classification: `class_ is None` (not an object name at all) and `class_` names
    a namespace with no backend support yet (_CLASS_TO_KIND maps it to None).

    Raises KeyError if `class_` is a non-None string this function has never seen - see
    _CLASS_TO_KIND's own comment for why that is the deliberately loud failure mode instead of
    silently returning None (which would look identical to "not an object name" or "known but
    unsupported namespace" and hide a real registry change).
    """
    if class_ is None:
        return None
    explicit = _EXPLICIT_OVERRIDES.get((command_name, field_name))
    if explicit is not None:
        return explicit
    if class_ not in _CLASS_TO_KIND:
        raise KeyError(
            f"{command_name}.{field_name}: unrecognized gl.xml class=\"{class_}\" - add it to "
            f"object_name_lifecycle.py's _CLASS_TO_KIND (mapped to a GLObjectKind if this "
            f"backend should support it, or to None if not yet)"
        )
    kind = _CLASS_TO_KIND[class_]
    if kind is None:
        return None
    return ObjectNameOverride(kind=kind, role=_infer_role(command_name))
