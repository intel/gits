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
# '')`, e.g. "vertex array" -> "VertexArray", "program pipeline" -> "ProgramPipeline"). Auditing
# every gl.xml command confirmed this (see docs/development/modern-opengl-backend-
# architecture.md's auto-derivation decision for the full audit results) - zero disagreements
# between the old 80-entry hand table and (class_, naming) auto-derivation across every field
# the old table covered.
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
# metadata itself cannot resolve (see _EXPLICIT_OVERRIDES's own header comment) - "no override
# -> stays disabled" remains the structural default, just now with almost nothing left that
# needs a manual override at all.

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

    FRAMEBUFFER/VERTEX_ARRAY do have entries below now (see
    docs/development/opengl2-player-skeleton-plan.md's Decision 10) - both kinds were declared
    here (and in the C++ enum) ahead of their first real use precisely so the bucketed
    service's shape would not need to change again the moment a Gen command for either was
    enabled. (RENDERBUFFER followed the same path later - see glFramebufferRenderbuffer's
    LOOKUP once it gained a real, already-`enabled` caller.)

    QUERY was added for glGenQueries/glDeleteQueries (see modern-opengl-backend-architecture.md's
    "Reduce replay warning noise" decision) - both are exercised by every tested application (the
    g-truc ogl-samples framework's `test` base class creates one timer query per window and
    deletes it on shutdown). Per the GL spec's "Shared Objects and Multiple Contexts" rule (see
    shareGroupService.h's own header comment), query objects are container objects never shared
    between contexts, like FRAMEBUFFER/VERTEX_ARRAY/PROGRAM_PIPELINE/TRANSFORM_FEEDBACK below -
    see recorder/include/handleArgumentUpdaters.h's GroupIdentityFor(), which buckets this kind
    by per-context identity for exactly this reason.
    """
    TEXTURE = 'Texture'
    BUFFER = 'Buffer'
    PROGRAM = 'Program'
    SHADER = 'Shader'
    FRAMEBUFFER = 'Framebuffer'
    VERTEX_ARRAY = 'VertexArray'
    QUERY = 'Query'
    # glBindRenderbuffer/glBindSampler/glBindProgramPipeline were already `enabled=True`
    # (scalar-only) with zero lifecycle tracking for their own object namespace, so their GLuint
    # name was passed straight through to the replay driver unresolved.
    RENDERBUFFER = 'Renderbuffer'
    SAMPLER = 'Sampler'
    # Per the GL spec's "Shared Objects and Multiple Contexts" rule (see
    # shareGroupService.h's own header comment), program pipeline objects are, like
    # FRAMEBUFFER/VERTEX_ARRAY/QUERY, container objects never shared between contexts - unlike
    # RENDERBUFFER/SAMPLER above, which (per that same rule) genuinely are shared. See
    # recorder/include/handleArgumentUpdaters.h's GroupIdentityFor(), which buckets this kind
    # by per-context identity for exactly this reason.
    PROGRAM_PIPELINE = 'ProgramPipeline'
    # glGenTransformFeedbacks/glDeleteTransformFeedbacks/glBindTransformFeedback/
    # glIsTransformFeedback/glDrawTransformFeedback(Stream(Instanced)). Per the GL spec's
    # "Shared Objects and Multiple Contexts" rule, transform feedback objects are, like
    # FRAMEBUFFER/VERTEX_ARRAY/QUERY/PROGRAM_PIPELINE above, container objects never shared
    # between contexts - see recorder/include/handleArgumentUpdaters.h's GroupIdentityFor(),
    # which buckets this kind by per-context identity for exactly this reason.
    TRANSFORM_FEEDBACK = 'TransformFeedback'


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
                        # glUseProgram/glBindAttribLocation/glGetUniformLocation's object-name
                        # parameter.
    DELETE = 'delete'   # an existing name (or names) destroyed on success, looked up first -
                        # glDeleteTextures/glDeleteProgram/glDeleteShader.
    # A LOOKUP that is *also* a legal CREATE the instant the name has never been seen before -
    # see _EXPLICIT_OVERRIDES's own comment on exactly which commands this applies to and why
    # (the GL spec text itself, not a heuristic). A miss here may be the real driver
    # implicitly creating a brand-new object with this exact name, or (a core profile's
    # texture/buffer bind) an invalid bind. The recorder decides after the real call: if the
    # entry's is_command reports the object exists, it registers a fresh key like an ordinary
    # CREATE-role field's Register(); otherwise it is an ordinary LOOKUP miss (see
    # recorder/include/handleArgumentUpdaters.h's UpdateHandleIfKnown/ConfirmImplicitCreate).
    LOOKUP_OR_CREATE = 'lookup_or_create'


@dataclass(frozen=True, kw_only=True)
class ObjectNameOverride:
    # None only together with identifier_param below.
    kind: GLObjectKind | None
    role: ObjectNameRole
    # LOOKUP_OR_CREATE only, both required: the same entry-point family's Gen command, which the
    # player uses to create the replay-time name (an APPLE vertex array name is not valid for
    # core glBindVertexArray, nor the reverse), and Is command, which the recorder uses after
    # the real bind to confirm the object was created (a core profile rejects an unknown
    # texture or buffer name instead).
    gen_command: str | None = None
    is_command: str | None = None
    # The sibling GLenum target parameter that decides the namespace at runtime: GL_RENDERBUFFER
    # names a renderbuffer, any other target a texture (see ImageObjectKind in
    # objectNameMapService.h). `kind` is then TEXTURE.
    target_param: str | None = None
    # The sibling GLenum identifier parameter (group="ObjectIdentifier": GL_BUFFER, GL_TEXTURE,
    # ...) that decides the namespace at runtime (see ObjectIdentifierKind in
    # objectNameMapService.h). `kind` is then None. GL_DISPLAY_LIST names are not mapped (see
    # ObjectIdentifierMapsNames).
    identifier_param: str | None = None

    def __post_init__(self) -> None:
        if (self.identifier_param is not None) != (self.kind is None):
            raise ValueError(
                f'{self.kind}: kind must be None exactly when identifier_param is set')
        if self.identifier_param is not None and self.role != ObjectNameRole.LOOKUP:
            raise ValueError('identifier_param is only defined for an ObjectNameRole.LOOKUP')
        implicit_create = self.role == ObjectNameRole.LOOKUP_OR_CREATE
        if implicit_create != (self.gen_command is not None and self.is_command is not None):
            raise ValueError(
                f'{self.kind}: gen_command and is_command are required for, and only for, '
                f'ObjectNameRole.LOOKUP_OR_CREATE')
        if self.target_param is not None and (self.kind != GLObjectKind.TEXTURE or
                                              self.role != ObjectNameRole.LOOKUP):
            raise ValueError(
                f'{self.kind}: target_param is only defined for a TEXTURE-kind '
                f'ObjectNameRole.LOOKUP')


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
    'transform feedback': GLObjectKind.TRANSFORM_FEEDBACK,
    # No GLObjectKind/ObjectNameMapService support yet - no tested sample needs any of these
    # remaining namespaces (see modern-opengl-backend-architecture.md's deferred-scope section
    # for why each stays deferred). Listed explicitly (not just absent) so a field tagged with
    # one of these is a *documented*, deliberate "not yet" rather than indistinguishable from a
    # gl.xml class= value nobody has ever looked at.
    'sync': None,
    'display list': None,
    'perf query handle': None,
    'perf query id': None,
}

# GL_ARB_shader_objects' GLhandleARB fields carry no class= in gl.xml. Those of a command with a
# core <alias> take the class of the alias's field at the same position (glAttachObjectARB's
# containerObj/obj -> glAttachShader's program/shader - see intermediates_creator.py's
# _inherit_handle_arb_classes); these are the commands without one. Their handle may name a
# shader or a program: both share one namespace (GL 2.0 "Shader Objects"), and
# ObjectNameMapService keeps GLObjectKind::Shader and GLObjectKind::Program in one bucket, so
# 'program' resolves either.
HANDLE_ARB_CLASSES: dict[tuple[str, str], str] = {
    ('glDeleteObjectARB', 'obj'): 'program',
    ('glGetInfoLogARB', 'obj'): 'program',
    ('glGetObjectParameterfvARB', 'obj'): 'program',
    ('glGetObjectParameterivARB', 'obj'): 'program',
}

# GLhandleARB fields captured as a plain value: glGetHandleARB returns the current program, a
# query result nothing on replay reads. Every other GLhandleARB field needs a class (see
# intermediates_creator.py's _requires_object_name_override).
UNMAPPED_HANDLE_ARB_FIELDS: frozenset[tuple[str, str]] = frozenset({
    ('glGetHandleARB', 'return_value'),
})

# gl.xml classes whose recorded names are replayed verbatim, without ObjectNameMapService. Display
# lists: glNewList defines any unused name (glGenLists only reserves a range), and on replay only
# the stream itself creates lists, so every recorded name is a valid replay-time name. Identity
# also keeps glListBase offsets and glCallLists' list arrays (base + offset arithmetic) correct,
# which per-name remapping could not.
IDENTITY_NAME_CLASSES: frozenset[str] = frozenset({'display list'})

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
# glIs<Object> existence queries. Residency queries (glIsNamedBufferResidentNV) test a property
# of an existing object, so their GL_FALSE does not mean the name is unused.
_IS_RE = re.compile(r'^glIs(?!.*Resident)[A-Z]')


def is_existence_query(command_name: str, return_type: str, override: ObjectNameOverride) -> bool:
    """Whether `override` is the object name a glIs* query tests (glIsTexture's `texture`, ...).
    Such a lookup keeps key 0 for an unregistered name the driver reports as no object - see
    ConfirmExistenceQuery (recorder/include/handleArgumentUpdaters.h)."""
    return (bool(_IS_RE.match(command_name)) and return_type.strip() == 'GLboolean'
            and override.role is ObjectNameRole.LOOKUP)


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

    # ObjectNameRole.LOOKUP_OR_CREATE: commands whose own GL/extension spec text says binding a
    # name that does not yet exist implicitly creates a new object and assigns it that exact
    # name, rather than requiring a prior Gen call (auto-derivation has no way to know this from
    # `class=`/naming alone - "Bind*" always infers plain LOOKUP otherwise, which is correct for
    # the vast majority of Bind commands that really do require a pre-existing name). This was a
    # real, concrete bug: a real target application's captured OpenGL stream - a GL 3.2
    # COMPATIBILITY profile app (compatibility profile keeps every one of these implicit-create
    # paths legal) - binds hundreds of textures by name with no corresponding glGenTextures ever
    # observed by the recorder, each one producing an ObjectNameMapService "no registered key"
    # warning and, on replay, a skipped call (see SkipUnmappedObjectNameCall's own comment) -
    # not a startup race, not a missing EXT/ARB alias (see the pointer_metadata.py
    # glGenTexturesEXT/glGenQueriesEXT fix for that separate, already-covered gap), just this
    # GL-spec-legal calling pattern the backend had no representation for at all.
    #
    # Base GL 1.0, not extension-gated - see glBindTexture's own man page: "If <texture> is not
    # the name of an existing texture object, a new texture object named <texture> is created
    # ...". Compatibility profile only; the core profile (GL 3.1+) rejects names glGenTextures
    # never returned with GL_INVALID_OPERATION, which is why the recorder confirms creation
    # with is_command after the bind instead of assuming it:
    ('glBindTexture', 'texture'): ObjectNameOverride(
        kind=GLObjectKind.TEXTURE, role=ObjectNameRole.LOOKUP_OR_CREATE,
        gen_command='glGenTextures', is_command='glIsTexture'),
    ('glBindTextureEXT', 'texture'): ObjectNameOverride(
        kind=GLObjectKind.TEXTURE, role=ObjectNameRole.LOOKUP_OR_CREATE,
        gen_command='glGenTexturesEXT', is_command='glIsTextureEXT'),

    # Core GL 1.5/ARB_vertex_buffer_object: binding a name glGenBuffers never returned creates a
    # buffer object with that name. Still legal in the compatibility profile; the core profile
    # (GL 3.1+) rejects it with GL_INVALID_OPERATION, which the recorder's is_command check
    # after the bind detects. glBindBufferARB needs its own entry - this table does not follow
    # gl.xml aliases.
    ('glBindBuffer', 'buffer'): ObjectNameOverride(
        kind=GLObjectKind.BUFFER, role=ObjectNameRole.LOOKUP_OR_CREATE,
        gen_command='glGenBuffers', is_command='glIsBuffer'),
    ('glBindBufferARB', 'buffer'): ObjectNameOverride(
        kind=GLObjectKind.BUFFER, role=ObjectNameRole.LOOKUP_OR_CREATE,
        gen_command='glGenBuffersARB', is_command='glIsBufferARB'),

    # GL_EXT_framebuffer_object (both the framebuffer and renderbuffer halves) - spec'd with the
    # identical "if <framebuffer>/<renderbuffer> is not zero, and if it is not the name of an
    # existing ... object, a new ... object is created" text as glBindTexture above. The later
    # ARB-promoted core glBindFramebuffer/glBindRenderbuffer deliberately keep plain,
    # auto-derived LOOKUP here - ARB_framebuffer_object's own spec text requires a pre-existing
    # name from Gen*, dropping the EXT version's implicit-create behavior on purpose.
    #
    # NOT the same problem as the legacy OpenGL backend's handleFBOExtMixing()/
    # isSchedulefboEXTAsCoreWA() (OpenGL/common/include/mapping.h, OpenGL/common/openglTools.cpp)
    # - an earlier version of this comment wrongly claimed it was; corrected after being
    # directly challenged, so recording the actual distinction here instead of just quietly
    # fixing it. Those two legacy mechanisms exist because the legacy backend gives EXT and core
    # FBO/RBO commands two SEPARATE C++ map classes (CGLFramebufferEXT vs CGLFramebuffer) -
    # ScheduleFboEXTAsCoreWA (opt-in, default OFF) rewrites the app's EXT calls into core ones in
    # the recorded stream itself so everything lands in one map; handleFBOExtMixing() is the
    # always-on fallback for when that option is off, letting a core-map miss fall back to
    # checking the EXT map too. It is deliberately opt-in, not the default, because silently
    # rewriting which API entry point the app actually called changes what is being validated -
    # a real concern for a driver-validation tool, where an EXT vs. core entry point can legally
    # take a different internal driver path even for equivalent GL semantics.
    # This backend has no such problem to work around in the first place: _CLASS_TO_KIND above
    # maps gl.xml's class="framebuffer"/"renderbuffer" straight to one single GLObjectKind
    # bucket regardless of which command spells it EXT or core, so glGenFramebuffersEXT and
    # glGenFramebuffers already register into (and glBindFramebufferEXT/glBindFramebuffer
    # already look up from) the exact same ObjectNameMapService bucket - an app mixing EXT and
    # core FBO/RBO calls for the same object already resolves correctly with zero special-casing,
    # and this backend never rewrites which entry point was actually called on the wire or at
    # replay either. LOOKUP_OR_CREATE below is solving a completely different, narrower problem:
    # an app that never calls ANY Gen variant at all and only ever binds via the EXT entry point
    # directly - legal per GL_EXT_framebuffer_object's own spec text quoted above, independent of
    # whether that same app also happens to mix in core calls elsewhere.
    ('glBindFramebufferEXT', 'framebuffer'): ObjectNameOverride(
        kind=GLObjectKind.FRAMEBUFFER, role=ObjectNameRole.LOOKUP_OR_CREATE,
        gen_command='glGenFramebuffersEXT', is_command='glIsFramebufferEXT'),
    ('glBindRenderbufferEXT', 'renderbuffer'): ObjectNameOverride(
        kind=GLObjectKind.RENDERBUFFER, role=ObjectNameRole.LOOKUP_OR_CREATE,
        gen_command='glGenRenderbuffersEXT', is_command='glIsRenderbufferEXT'),

    # GL_APPLE_vertex_array_object - same "if <array> is not the name of an existing vertex
    # array object, a new vertex array object ... is created" text as the FBO extensions above;
    # the later ARB-promoted core glBindVertexArray again requires a pre-existing name from
    # glGenVertexArrays and stays plain LOOKUP.
    #
    # glBindVertexArrayOES (GL_OES_vertex_array_object, the ES-side promotion) deliberately has
    # NO override here, unlike its APPLE sibling above - gl.xml gives it
    # <alias name="glBindVertexArray"/>, Khronos's own declaration that this command is the
    # identical command as core glBindVertexArray, not merely a same-shaped one - so it already
    # auto-derives the correct plain LOOKUP (requires a pre-existing glGenVertexArraysOES/
    # glGenVertexArrays name, exactly like core) without needing an override at all. An earlier
    # version of this file gave it LOOKUP_OR_CREATE anyway, by pattern-matching it to the APPLE
    # extension above without checking gl.xml's own alias for it first - caught only when
    # directly asked "should EXT and core really share one map/namespace" prompted rechecking
    # every one of this section's entries against gl.xml's <alias> tags specifically (see
    # glBindFramebufferEXT's own comment above for the fuller cross-check: every Gen/Delete/Is
    # EXT command in this whole file that IS aliased to a core command needs no override, and
    # every Bind command that overrides to LOOKUP_OR_CREATE correctly has NO alias - this was
    # the one entry that broke that pattern).
    ('glBindVertexArrayAPPLE', 'array'): ObjectNameOverride(
        kind=GLObjectKind.VERTEX_ARRAY, role=ObjectNameRole.LOOKUP_OR_CREATE,
        gen_command='glGenVertexArraysAPPLE', is_command='glIsVertexArrayAPPLE'),

    # GL 1.5 / ARB_occlusion_query: "If BeginQuery is called with an unused id, that name is
    # marked as used and associated with a new query object." Compatibility drivers still accept
    # it, and so do glBeginQueryIndexed and glQueryCounter on them; applications start queries
    # this way without ever calling glGenQueries. The is_command check after the call keeps a
    # driver that rejects the name an ordinary lookup miss.
    ('glBeginQuery', 'id'): ObjectNameOverride(
        kind=GLObjectKind.QUERY, role=ObjectNameRole.LOOKUP_OR_CREATE,
        gen_command='glGenQueries', is_command='glIsQuery'),
    ('glBeginQueryARB', 'id'): ObjectNameOverride(
        kind=GLObjectKind.QUERY, role=ObjectNameRole.LOOKUP_OR_CREATE,
        gen_command='glGenQueriesARB', is_command='glIsQueryARB'),
    ('glBeginQueryIndexed', 'id'): ObjectNameOverride(
        kind=GLObjectKind.QUERY, role=ObjectNameRole.LOOKUP_OR_CREATE,
        gen_command='glGenQueries', is_command='glIsQuery'),
    ('glQueryCounter', 'id'): ObjectNameOverride(
        kind=GLObjectKind.QUERY, role=ObjectNameRole.LOOKUP_OR_CREATE,
        gen_command='glGenQueries', is_command='glIsQuery'),

    # gl.xml leaves glCopyImageSubData's names untagged, as their target decides whether they
    # name a texture or a renderbuffer. Replayed verbatim, they would copy between other objects
    # whenever replay names differ from the recorded ones, as they do in a subcapture.
    **{(command, f'{side}Name'): ObjectNameOverride(
        kind=GLObjectKind.TEXTURE, role=ObjectNameRole.LOOKUP, target_param=f'{side}Target')
       for command in ('glCopyImageSubData', 'glCopyImageSubDataEXT', 'glCopyImageSubDataNV',
                       'glCopyImageSubDataOES')
       for side in ('src', 'dst')},

    # gl.xml leaves glObjectLabel's name untagged, as its identifier decides the namespace.
    **{(command, 'name'): ObjectNameOverride(
        kind=None, role=ObjectNameRole.LOOKUP, identifier_param='identifier')
       for command in ('glObjectLabel', 'glObjectLabelKHR')},
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
    GLuint like a texture unit index or an enum-typed identifier, see e.g. glObjectLabel's
    name, which the registry itself deliberately leaves untagged because its real namespace is
    only known at runtime from a sibling GLenum). An _EXPLICIT_OVERRIDES entry wins even then
    (glCopyImageSubData's srcName). Otherwise returns None for exactly the same two cases
    _requires_object_name_override's caller needs to distinguish from a real classification:
    `class_ is None` (not an object name at all) and `class_` names a namespace with no backend
    support yet (_CLASS_TO_KIND maps it to None).

    Raises KeyError if `class_` is a non-None string this function has never seen - see
    _CLASS_TO_KIND's own comment for why that is the deliberately loud failure mode instead of
    silently returning None (which would look identical to "not an object name" or "known but
    unsupported namespace" and hide a real registry change).
    """
    explicit = _EXPLICIT_OVERRIDES.get((command_name, field_name))
    if explicit is not None:
        return explicit
    if class_ is None:
        return None
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
