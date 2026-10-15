# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

"""Tests for intermediates_creator.py's systemic "GLuint object name requires a resolvable
object-name classification to stay enabled" gate (see _requires_object_name_override's own
docstring), an enforced rule the generator itself checks on every run rather than relying on a
one-off, per-command audit. Since the object-name-auto-
derivation rewrite (see object_name_lifecycle.py's header comment), "resolvable" almost always
means auto-derived from gl.xml's own class="..." annotation plus the command's name, not a
hand-written table entry - these tests exercise the gate itself, not the (now much smaller)
manual exception table.
"""

import sys
import unittest
from pathlib import Path

CODEGEN_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(CODEGEN_DIR))

from generator_helpers import object_name_override_for
from intermediates import Api, EnumType, EnumValue, Parameter, ReturnValue
from intermediates_creator import (
    FILE_TO_API,
    EnumKey,
    _is_fully_capturable_command,  # pyright: ignore[reportPrivateUsage]
    _is_param_capturable,  # pyright: ignore[reportPrivateUsage]
    _requires_object_name_override,  # pyright: ignore[reportPrivateUsage]
    build_enum_name_map,
    extract_commands_from_registry,
    get_commands_from_xml_data,
)
from object_name_lifecycle import GLObjectKind, ObjectNameRole
from registry import Registry

REGISTRY_PATHS: dict[Api, Path] = {api: path for path, api in FILE_TO_API.items()}


def load_registry(api: Api) -> Registry:
    registry = Registry()
    registry.loadFile(REGISTRY_PATHS[api])
    return registry


def get_command(api: Api, name: str):
    registry = load_registry(api)
    raw_commands = extract_commands_from_registry(registry)
    raw = next(c for c in raw_commands if c['command_attribs']['name'] == name)
    return get_commands_from_xml_data({api: [raw]})[api][0]


class SpecialNumbersTests(unittest.TestCase):
    def test_tokens_with_other_groups_are_available_for_lookup(self) -> None:
        # If an enum has multiple groups, ignore the 'SpecialNumbers' group if present.
        # If an enum has only the `SpecialNumbers` group, ignore it entirely.
        names = build_enum_name_map({Api.GL: [
            EnumValue(name='GL_NO_ERROR', value='0',
                      groups=('SpecialNumbers', 'GraphicsResetStatus', 'ErrorCode')),
            EnumValue(name='GL_ZERO', value='0',
                      groups=('SpecialNumbers', 'BlendingFactor')),
            EnumValue(name='GL_ONE', value='1',
                      groups=('SpecialNumbers', 'BlendingFactor')),
            EnumValue(name='GL_UUID_SIZE_EXT', value='16', groups=('SpecialNumbers',)),
            EnumValue(name='GL_INVALID_INDEX', value='0xFFFFFFFF', type=EnumType.UINT,
                      groups=('SpecialNumbers',)),
        ]})

        self.assertEqual(names[EnumKey(Api.GL, 0, 'ErrorCode')], 'GL_NO_ERROR')
        self.assertEqual(names[EnumKey(Api.GL, 0, 'BlendingFactor')], 'GL_ZERO')
        self.assertEqual(names[EnumKey(Api.GL, 1, 'BlendingFactor')], 'GL_ONE')
        self.assertFalse(any(key.group == 'SpecialNumbers' for key in names))
        self.assertNotIn(EnumKey(Api.GL, 16, ''), names)


class RequiresObjectNameOverrideTests(unittest.TestCase):
    """Direct unit tests for the gate predicate itself, independent of registry data."""

    def test_untagged_field_never_requires_an_entry(self) -> None:
        param = Parameter(name='count', type='GLsizei')
        self.assertFalse(_requires_object_name_override('glSomeCommand', param))

    def test_tagged_field_with_unsupported_kind_requires_one(self) -> None:
        # class_="perf query handle" is a real gl.xml value auto-derivation deliberately cannot
        # resolve - this backend has no GLObjectKind/ObjectNameMapService support for perf
        # query handles yet (see object_name_lifecycle.py's _CLASS_TO_KIND) - unaffected by
        # naming, since resolve_object_name_override() checks kind support before it ever
        # computes a role. (class_="sync"/GLsync used to be this test's example, before GLsync
        # became a mapped handle - see test_mapped_handle_field_bypasses_the_gate_even_with_an_
        # unsupported_class below for its current, opposite behavior.)
        param = Parameter(name='queryHandle', type='GLuint', class_='perf query handle')
        self.assertTrue(_requires_object_name_override('glSomeHypotheticalCommand', param))

    def test_mapped_handle_field_bypasses_the_gate_even_with_an_unsupported_class(self) -> None:
        # GLsync fields are always tagged class_="sync" by gl.xml, and 'sync' resolves to no
        # GLObjectKind (object_name_lifecycle.py's _CLASS_TO_KIND) - by the same logic as
        # test_tagged_field_with_unsupported_kind_requires_one above, this would normally
        # require an (unavailable) object-name override. is_mapped_handle fields are exempted
        # from the gate entirely instead: GLsync gets a real mapping of its own
        # (SyncHandleMapService), not through ObjectNameMapService - see
        # _requires_object_name_override's own docstring and intermediates.py's
        # _MAPPED_HANDLE_BASE_TYPES.
        param = Parameter(name='sync', type='GLsync', class_='sync')
        self.assertFalse(_requires_object_name_override('glSomeHypotheticalCommand', param))
        # HGLRC has no gl.xml class_ at all (it is a WGL type, not a GL registry object name),
        # but the same bypass applies to it unconditionally for the same reason.
        ctx_param = Parameter(name='hglrc', type='HGLRC')
        self.assertFalse(_requires_object_name_override('glSomeHypotheticalCommand', ctx_param))

    def test_tagged_field_with_supported_kind_auto_derives(self) -> None:
        # class_="texture" auto-derives (kind=TEXTURE, role=LOOKUP - see
        # object_name_lifecycle.py's resolve_object_name_override) for ANY command name that
        # does not match the Gen*/Create*/Delete* naming heuristic, not just glBindTexture
        # specifically - this is the whole point of auto-derivation over a per-command table.
        param = Parameter(name='texture', type='GLuint', class_='texture')
        self.assertFalse(_requires_object_name_override('glBindTexture', param))
        self.assertFalse(_requires_object_name_override('glSomeHypotheticalLookupCommand', param))

    def test_return_value_field_is_checked_the_same_way(self) -> None:
        # Unsupported kind: still requires an entry regardless of command name.
        ret_unsupported_kind = ReturnValue(type='GLuint', class_='perf query handle')
        self.assertTrue(
            _requires_object_name_override('glSomeHypotheticalCommand', ret_unsupported_kind))
        # Mapped-handle return value: bypassed regardless of its (otherwise-unsupported) class_
        # - this is glFenceSync's real shape (GLsync return, class_="sync").
        ret_mapped_handle = ReturnValue(type='GLsync', class_='sync')
        self.assertFalse(
            _requires_object_name_override('glFenceSync', ret_mapped_handle))
        # Supported kind, CREATE-shaped command name: auto-derives (glCreateProgram really does
        # return a newly-created PROGRAM name).
        ret_with_entry = ReturnValue(type='GLuint', class_='program')
        self.assertFalse(_requires_object_name_override('glCreateProgram', ret_with_entry))


class IsParamCapturableGateTests(unittest.TestCase):
    """_is_param_capturable must reject a class-tagged field lacking a table entry in every
    branch it could otherwise fall into (scalar, and array-element-eligible)."""

    def test_scalar_object_name_without_override_is_not_capturable(self) -> None:
        # class_="perf query handle" has no GLObjectKind/ObjectNameMapService support - stays
        # unresolvable regardless of command name (see _CLASS_TO_KIND's own comment); "texture"
        # would no longer demonstrate this, since it now auto-derives for ANY command name.
        # (class_="sync"/GLsync used to be this test's example - see
        # test_mapped_handle_param_is_capturable_despite_unsupported_class below for why GLsync
        # itself is now capturable instead.)
        param = Parameter(name='queryHandle', type='GLuint', class_='perf query handle')
        self.assertFalse(_is_param_capturable('glSomeHypotheticalCommand', param, []))

    def test_mapped_handle_param_is_capturable_despite_unsupported_class(self) -> None:
        # GLsync's real shape (glIsSync/glClientWaitSync/glWaitSync/glDeleteSync's `sync`
        # parameter): class_="sync" resolves to no GLObjectKind, but is_mapped_handle bypasses
        # the object-name gate entirely (see test_mapped_handle_field_bypasses_the_gate_even_
        # with_an_unsupported_class above) - facts.is_mapped_handle alone is then enough for
        # _is_param_capturable's own scalar/opaque-handle/mapped-handle branch to accept it.
        param = Parameter(name='sync', type='GLsync', class_='sync')
        self.assertTrue(_is_param_capturable('glDeleteSync', param, []))

    def test_scalar_object_name_with_override_is_capturable(self) -> None:
        param = Parameter(name='texture', type='GLuint', class_='texture')
        self.assertTrue(_is_param_capturable('glBindTexture', param, []))

    def test_scalar_non_object_name_is_unaffected(self) -> None:
        # A plain GLenum/GLsizei/etc. with no class_ never even reaches the gate.
        param = Parameter(name='target', type='GLenum')
        self.assertTrue(_is_param_capturable('glBindTexture', param, []))

    def test_array_element_eligible_object_name_without_override_is_not_capturable(self) -> None:
        # Fabricated: has a real POINTER_PARAM_OVERRIDES-shaped sibling ('n'), and would be
        # array_element_eligible/POINTER_PARAM_OVERRIDES-satisfied on its own, but its class_
        # tag has no matching OBJECT_NAME_PARAM_OVERRIDES entry for this made-up command name -
        # must still be rejected, mirroring the real glDeleteBuffers-shaped gap this gate was
        # written to catch (see object_name_lifecycle.py's own comment).
        param = Parameter(name='buffers', type='const GLuint *', class_='buffer')
        self.assertFalse(_is_param_capturable('glDeleteTextures', param, []))

    def test_array_element_eligible_object_name_with_override_is_capturable(self) -> None:
        # glDeleteTextures.textures really is both POINTER_PARAM_OVERRIDES- and
        # OBJECT_NAME_PARAM_OVERRIDES-covered. Its POINTER_PARAM_OVERRIDES entry names a
        # sibling length parameter 'n', which must also be present in other_params for
        # _validate_pointer_override's own (unrelated) structural check to pass.
        param = Parameter(name='textures', type='const GLuint *', class_='texture')
        sibling = Parameter(name='n', type='GLsizei')
        self.assertTrue(_is_param_capturable('glDeleteTextures', param, [sibling]))


class EndToEndSystemicGateTests(unittest.TestCase):
    """Confirms the gate's effect against real registry data: commands with a resolvable
    object-name classification come out enabled with the right
    override, and commands deliberately left in the deferred list stay disabled."""

    def test_glbindtextureunit_is_now_enabled(self) -> None:
        # A command with a resolvable object-name classification that was previously unmapped.
        cmd = get_command(Api.GL, 'glBindTextureUnit')
        self.assertTrue(cmd.enabled)
        texture_param = next(p for p in cmd.params if p.name == 'texture')
        self.assertEqual(texture_param.class_, 'texture')

    def test_quick_win_lookup_commands_are_enabled(self) -> None:
        for cmd_name, param_name, kind in (
            ('glIsTexture', 'texture', GLObjectKind.TEXTURE),
            ('glIsBuffer', 'buffer', GLObjectKind.BUFFER),
            ('glIsProgram', 'program', GLObjectKind.PROGRAM),
            ('glIsShader', 'shader', GLObjectKind.SHADER),
            ('glIsFramebuffer', 'framebuffer', GLObjectKind.FRAMEBUFFER),
            ('glIsRenderbuffer', 'renderbuffer', GLObjectKind.RENDERBUFFER),
            ('glIsVertexArray', 'array', GLObjectKind.VERTEX_ARRAY),
            ('glIsQuery', 'id', GLObjectKind.QUERY),
            ('glIsSampler', 'sampler', GLObjectKind.SAMPLER),
            ('glIsProgramPipeline', 'pipeline', GLObjectKind.PROGRAM_PIPELINE),
            ('glFramebufferRenderbuffer', 'renderbuffer', GLObjectKind.RENDERBUFFER),
            ('glUniformBlockBinding', 'program', GLObjectKind.PROGRAM),
            ('glShaderStorageBlockBinding', 'program', GLObjectKind.PROGRAM),
            ('glTextureParameteri', 'texture', GLObjectKind.TEXTURE),
            ('glTextureParameterf', 'texture', GLObjectKind.TEXTURE),
        ):
            with self.subTest(command=cmd_name):
                cmd = get_command(Api.GL, cmd_name)
                self.assertTrue(cmd.enabled)
                param = next(p for p in cmd.params if p.name == param_name)
                self.assertIsNotNone(param.class_)
                override = object_name_override_for(cmd_name, param_name, param.class_)
                self.assertIsNotNone(override)
                assert override is not None
                self.assertEqual(override.kind, kind)
                self.assertEqual(override.role, ObjectNameRole.LOOKUP)

    def test_sync_object_commands_are_enabled(self) -> None:
        # glFenceSync/glIsSync/glClientWaitSync/glWaitSync/glDeleteSync all take/return a
        # GLsync (class_="sync" in gl.xml) - previously disabled entirely (no GLObjectKind for
        # 'sync', and GLsync was not yet a mapped handle), now enabled via
        # intermediates.py's _MAPPED_HANDLE_BASE_TYPES + this module's is_mapped_handle bypass
        # in _requires_object_name_override. Every `sync`-named field here resolves through
        # SyncHandleMapService (a HandleMapService<GLsync> alias - see common/services/
        # handleMapService.h), never through
        # object_name_override_for - confirmed by asserting no override for the 'sync' field,
        # unlike test_quick_win_lookup_commands_are_enabled's GLuint-named-object commands
        # above.
        for cmd_name, sync_param_name in (
            ('glIsSync', 'sync'),
            ('glClientWaitSync', 'sync'),
            ('glWaitSync', 'sync'),
            ('glDeleteSync', 'sync'),
        ):
            with self.subTest(command=cmd_name):
                cmd = get_command(Api.GL, cmd_name)
                self.assertTrue(cmd.enabled)
                param = next(p for p in cmd.params if p.name == sync_param_name)
                self.assertEqual(param.class_, 'sync')
                self.assertTrue(param.type_facts.is_mapped_handle)
                self.assertIsNone(object_name_override_for(cmd_name, sync_param_name, param.class_))

        # glFenceSync's own shape: no `sync` parameter at all - its RETURN value is the new
        # mapped handle instead (see handle_lifecycle.py's GLSYNC_LIFECYCLE.creating_commands).
        fence_cmd = get_command(Api.GL, 'glFenceSync')
        self.assertTrue(fence_cmd.enabled)
        self.assertEqual(fence_cmd.return_value.class_, 'sync')
        self.assertTrue(fence_cmd.return_value.type_facts.is_mapped_handle)
        self.assertIsNone(
            object_name_override_for('glFenceSync', 'return_value', fence_cmd.return_value.class_))
        condition_param = next(p for p in fence_cmd.params if p.name == 'condition')
        flags_param = next(p for p in fence_cmd.params if p.name == 'flags')
        self.assertTrue(condition_param.type_facts.is_scalar)
        self.assertTrue(flags_param.type_facts.is_scalar)

    def test_deferred_scope_commands_stay_disabled(self) -> None:
        # These remain disabled after the gate for two structurally different reasons - see
        # modern-opengl-backend-architecture.md's deferred-scope section for the full list.
        # Note that most of the old "DSA named-object family, kind exists but no table entry
        # yet" category (e.g. glTextureStorage2D, glNamedFramebufferTexture) no longer belongs
        # here at all: auto-derivation resolves their object-name field(s) from class_+naming
        # exactly like every other LOOKUP does, with nothing left to add by hand - see
        # EndToEndAutoDerivationTests below for that expanded-coverage behavior instead.
        for cmd_name in (
            'glGenLists',            # display list - no GLObjectKind at all.
            'glCallList',            # display list.
            'glDeleteLists',         # display list.
            # glBindTransformFeedback/glIsTransformFeedback USED TO be listed here too
            # ("transform feedback - no GLObjectKind at all"). GLObjectKind.TRANSFORM_FEEDBACK
            # now exists (object_name_lifecycle.py), so both are now genuinely enabled - see
            # EndToEndAutoDerivationTests' test_transform_feedback_lifecycle_commands_are_now_enabled
            # below.
            'glBeginPerfQueryINTEL',  # perf query handle - no GLObjectKind at all.
            # Object-name field(s) auto-derive fine (kind IS supported), but a *different*,
            # independent parameter (a plain string/pointer with no POINTER_PARAM_OVERRIDES
            # entry of its own) keeps the whole command disabled - confirms auto-derivation
            # only ever *removes* a blocking reason, never masks an unrelated one.
            'glActiveVaryingNV',     # 'program' resolves; 'name' (GLchar*) has no entry.
            'glClearTexImage',       # 'texture' resolves; 'pixels' (void*) has no entry.
        ):
            with self.subTest(command=cmd_name):
                cmd = get_command(Api.GL, cmd_name)
                self.assertFalse(cmd.enabled)

    def test_glflushmappedbufferrange_is_force_disabled(self) -> None:
        # Structurally capturable (plain scalar target/offset/length, no
        # object-name gate involved at all - a distinct exclusion reason from every other test
        # in this class) but deliberately kept disabled - see
        # intermediates_creator.py's _FORCE_DISABLED_COMMANDS.
        cmd = get_command(Api.GL, 'glFlushMappedBufferRange')
        self.assertFalse(cmd.enabled)
        # Confirm it really would otherwise have been enabled, i.e. this is testing the
        # deliberate override and not some unrelated, coincidental disablement reason.
        for param in cmd.params:
            self.assertIsNone(param.class_)
            self.assertTrue(param.type_facts.is_scalar)

    def test_return_value_object_name_gate(self) -> None:
        # glCreateShaderProgramv's return value (class="program", CREATE-shaped name) auto-
        # derives fine on its own - the object-name gate is satisfied. The command USED TO stay
        # disabled anyway for a completely unrelated, independent reason: its `strings`
        # parameter is an array-of-arrays (const GLchar* const*) with no
        # ARRAY_OF_ARRAYS_PARAM_OVERRIDES entry (see array_of_arrays_metadata.py) - confirming
        # auto-derivation removed exactly the gap it was meant to and nothing more. That second
        # gap has since been closed too: array_of_arrays_metadata.py now has a `strings` entry
        # with element_length_param=None - gl.xml gives this command no `length` sibling at
        # all, so ArrayOfArrays<T>'s null-terminated-element-scan fallback is used instead - so
        # the command is now fully enabled.
        cmd = get_command(Api.GL, 'glCreateShaderProgramv')
        self.assertEqual(cmd.return_value.class_, 'program')
        self.assertFalse(
            _requires_object_name_override('glCreateShaderProgramv', cmd.return_value))
        self.assertTrue(cmd.enabled)

    def test_is_fully_capturable_command_rejects_unmapped_return_value(self) -> None:
        # class_="perf query handle" has no GLObjectKind/ObjectNameMapService support - stays
        # unresolvable regardless of command name (see _CLASS_TO_KIND's own comment).
        ret = ReturnValue(type='GLuint', class_='perf query handle')
        self.assertFalse(_is_fully_capturable_command('glSomeHypotheticalCommand', ret, []))

    def test_is_fully_capturable_command_accepts_mapped_handle_return_value(self) -> None:
        # glFenceSync's real shape: GLsync return (class_="sync", is_mapped_handle bypasses the
        # object-name gate), two plain GLenum/GLbitfield scalar params.
        ret = ReturnValue(type='GLsync', class_='sync')
        params = [Parameter(name='condition', type='GLenum'), Parameter(name='flags', type='GLbitfield')]
        self.assertTrue(_is_fully_capturable_command('glFenceSync', ret, params))


class EndToEndAutoDerivationTests(unittest.TestCase):
    """Confirms the object-name auto-derivation rewrite (see object_name_lifecycle.py's header
    comment) actually re-enables real, previously-deferred commands from real registry data -
    the concrete "how many of the 303 deferred commands get safely re-enabled" question this
    rewrite was built to answer. Mirrors EndToEndSystemicGateTests' pattern, one level up:
    that class tests the *gate*; this class tests what auto-derivation newly lets *through* it.
    """

    def test_dsa_named_object_lookup_commands_are_now_enabled(self) -> None:
        # None of these ever had a hand-written OBJECT_NAME_PARAM_OVERRIDES entry - each was
        # part of the "DSA named object family, kind exists, no entry yet" deferred bucket
        # test_deferred_scope_commands_stay_disabled used to list glTextureStorage2D/
        # glNamedFramebufferTexture under, before this rewrite closed that whole bucket at
        # once via class_+naming instead of one command at a time.
        for cmd_name, param_name, kind in (
            ('glTextureStorage2D', 'texture', GLObjectKind.TEXTURE),
            ('glNamedFramebufferTexture', 'framebuffer', GLObjectKind.FRAMEBUFFER),
            ('glNamedFramebufferTexture', 'texture', GLObjectKind.TEXTURE),
            ('glVertexArrayVertexBuffer', 'vaobj', GLObjectKind.VERTEX_ARRAY),
            ('glVertexArrayVertexBuffer', 'buffer', GLObjectKind.BUFFER),
            ('glNamedRenderbufferStorage', 'renderbuffer', GLObjectKind.RENDERBUFFER),
        ):
            with self.subTest(command=cmd_name, param=param_name):
                cmd = get_command(Api.GL, cmd_name)
                self.assertTrue(cmd.enabled)
                param = next(p for p in cmd.params if p.name == param_name)
                override = object_name_override_for(cmd_name, param_name, param.class_)
                self.assertIsNotNone(override)
                assert override is not None
                self.assertEqual(override.kind, kind)
                self.assertEqual(override.role, ObjectNameRole.LOOKUP)

    def test_free_object_buffer_ati_delete_exception_is_enabled(self) -> None:
        # The one real _EXPLICIT_OVERRIDES exception found by the audit backing this rewrite
        # (see object_name_lifecycle.py's own comment on this entry) - also exercised via real
        # registry data here, not just the fabricated-Parameter unit tests in
        # test_object_name_lifecycle.py's ResolveObjectNameOverrideTests.
        cmd = get_command(Api.GL, 'glFreeObjectBufferATI')
        self.assertTrue(cmd.enabled)

    def test_unsupported_kind_commands_stay_disabled_even_with_perfect_naming(self) -> None:
        # glCreatePerfQueryINTEL/glDeletePerfQueryINTEL both match the CREATE/DELETE naming
        # heuristic perfectly, but perf query id/handle objects have no GLObjectKind - confirms
        # naming alone never overrides kind support. (This test USED TO list
        # glGenTransformFeedbacks/glCreateTransformFeedbacks/glDeleteTransformFeedbacks here
        # instead: GLObjectKind.TRANSFORM_FEEDBACK now exists (object_name_lifecycle.py), so
        # that trio no longer demonstrates "unsupported kind" -
        # glGenTransformFeedbacks/glDeleteTransformFeedbacks are now genuinely enabled (see
        # test_transform_feedback_lifecycle_commands_are_now_enabled below); only
        # glCreateTransformFeedbacks itself still stays disabled, but for the unrelated,
        # missing-POINTER_PARAM_OVERRIDES-entry reason
        # test_create_transform_feedbacks_stays_disabled_pending_pointer_override covers.)
        for cmd_name in ('glCreatePerfQueryINTEL', 'glDeletePerfQueryINTEL'):
            with self.subTest(command=cmd_name):
                cmd = get_command(Api.GL, cmd_name)
                self.assertFalse(cmd.enabled)

    def test_transform_feedback_lifecycle_commands_are_now_enabled(self) -> None:
        # GLObjectKind.TRANSFORM_FEEDBACK now exists (object_name_lifecycle.py), so every
        # transform-feedback-object command auto-enables the same way any other supported
        # kind's commands do.
        # glGenTransformFeedbacks/glDeleteTransformFeedbacks additionally needed a new
        # POINTER_PARAM_OVERRIDES entry each (pointer_metadata.py) for their `ids` array; the
        # scalar-`id`-only lookup commands (glBindTransformFeedback/glIsTransformFeedback/
        # glDrawTransformFeedback(Instanced/Stream(Instanced))) needed no entries here at all -
        # the object-name gate alone was already enough for a plain scalar field.
        for cmd_name, param_name, role in (
            ('glGenTransformFeedbacks', 'ids', ObjectNameRole.CREATE),
            ('glDeleteTransformFeedbacks', 'ids', ObjectNameRole.DELETE),
            ('glBindTransformFeedback', 'id', ObjectNameRole.LOOKUP),
            ('glIsTransformFeedback', 'id', ObjectNameRole.LOOKUP),
            ('glDrawTransformFeedback', 'id', ObjectNameRole.LOOKUP),
            ('glDrawTransformFeedbackInstanced', 'id', ObjectNameRole.LOOKUP),
            ('glDrawTransformFeedbackStream', 'id', ObjectNameRole.LOOKUP),
            ('glDrawTransformFeedbackStreamInstanced', 'id', ObjectNameRole.LOOKUP),
        ):
            with self.subTest(command=cmd_name):
                cmd = get_command(Api.GL, cmd_name)
                self.assertTrue(cmd.enabled)
                param = next(p for p in cmd.params if p.name == param_name)
                override = object_name_override_for(cmd_name, param_name, param.class_)
                self.assertIsNotNone(override)
                assert override is not None
                self.assertEqual(override.kind, GLObjectKind.TRANSFORM_FEEDBACK)
                self.assertEqual(override.role, role)

    def test_create_transform_feedbacks_stays_disabled_pending_pointer_override(self) -> None:
        # Unlike glGenTransformFeedbacks/glDeleteTransformFeedbacks above, the DSA variant
        # glCreateTransformFeedbacks' own `ids` (GLuint *, CREATE role, no `n`-named sibling
        # consumed any differently) has no POINTER_PARAM_OVERRIDES entry - nobody added one,
        # since no tested sample calls this specific DSA entry point. The object-name gate is
        # already satisfied (kind auto-derives fine, exactly like its Gen/Delete siblings), so
        # this is purely the missing-pointer-classification reason, independent of kind
        # support - add a POINTER_PARAM_OVERRIDES entry once a real caller needs it.
        cmd = get_command(Api.GL, 'glCreateTransformFeedbacks')
        self.assertFalse(cmd.enabled)
        param = next(p for p in cmd.params if p.name == 'ids')
        self.assertEqual(param.class_, 'transform feedback')
        self.assertFalse(_requires_object_name_override('glCreateTransformFeedbacks', param))


if __name__ == '__main__':
    unittest.main()
