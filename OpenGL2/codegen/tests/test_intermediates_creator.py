# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

"""Tests for intermediates_creator.py's systemic "GLuint object name requires a resolvable
object-name classification to stay enabled" gate (see _requires_object_name_override's own
docstring) - the gate generalizing the earlier one-off, per-command audit into
an enforced rule the generator itself checks on every run. Since the object-name-auto-
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
from intermediates import Api, Parameter, ReturnValue
from intermediates_creator import (
    FILE_TO_API,
    _is_fully_capturable_command,  # pyright: ignore[reportPrivateUsage]
    _is_param_capturable,  # pyright: ignore[reportPrivateUsage]
    _requires_object_name_override,  # pyright: ignore[reportPrivateUsage]
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


class RequiresObjectNameOverrideTests(unittest.TestCase):
    """Direct unit tests for the gate predicate itself, independent of registry data."""

    def test_untagged_field_never_requires_an_entry(self) -> None:
        param = Parameter(name='count', type='GLsizei')
        self.assertFalse(_requires_object_name_override('glSomeCommand', param))

    def test_tagged_field_with_unsupported_kind_requires_one(self) -> None:
        # class_="sync" is a real gl.xml value auto-derivation deliberately cannot resolve -
        # this backend has no GLObjectKind/ObjectNameMapService support for sync objects yet
        # (see object_name_lifecycle.py's _CLASS_TO_KIND) - unaffected by naming, since
        # resolve_object_name_override() checks kind support before it ever computes a role.
        param = Parameter(name='sync', type='GLsync', class_='sync')
        self.assertTrue(_requires_object_name_override('glSomeHypotheticalCommand', param))

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
        ret_unsupported_kind = ReturnValue(type='GLsync', class_='sync')
        self.assertTrue(
            _requires_object_name_override('glSomeHypotheticalCommand', ret_unsupported_kind))
        # Supported kind, CREATE-shaped command name: auto-derives (glCreateProgram really does
        # return a newly-created PROGRAM name).
        ret_with_entry = ReturnValue(type='GLuint', class_='program')
        self.assertFalse(_requires_object_name_override('glCreateProgram', ret_with_entry))


class IsParamCapturableGateTests(unittest.TestCase):
    """_is_param_capturable must reject a class-tagged field lacking a table entry in every
    branch it could otherwise fall into (scalar, and array-element-eligible)."""

    def test_scalar_object_name_without_override_is_not_capturable(self) -> None:
        # class_="sync" has no GLObjectKind/ObjectNameMapService support - stays unresolvable
        # regardless of command name (see _CLASS_TO_KIND's own comment); "texture" would no
        # longer demonstrate this, since it now auto-derives for ANY command name.
        param = Parameter(name='sync', type='GLsync', class_='sync')
        self.assertFalse(_is_param_capturable('glSomeHypotheticalCommand', param, []))

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
        # written to catch (see object_name_lifecycle.py's own B.1 comment).
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
    """Confirms the gate's effect against real registry data: commands the gate
    targets (or that the quick-win overrides cover) come out enabled with the right
    override, and commands deliberately left in the deferred list stay disabled."""

    def test_glbindtextureunit_is_now_enabled(self) -> None:
        # The exact command found unmapped.
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
            'glBindTransformFeedback',  # transform feedback - no GLObjectKind at all.
            'glIsTransformFeedback',
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
        # glCreateShaderProgramv's return value (class="program", CREATE-shaped name) now
        # auto-derives fine on its own - the object-name gate is satisfied. The command stays
        # disabled anyway for a completely unrelated, independent reason: its `strings`
        # parameter is an array-of-arrays (const GLchar* const*) with no
        # ARRAY_OF_ARRAYS_PARAM_OVERRIDES entry (see array_of_arrays_metadata.py) - confirming
        # auto-derivation removed exactly the gap it was meant to and nothing more.
        cmd = get_command(Api.GL, 'glCreateShaderProgramv')
        self.assertEqual(cmd.return_value.class_, 'program')
        self.assertFalse(
            _requires_object_name_override('glCreateShaderProgramv', cmd.return_value))
        self.assertFalse(cmd.enabled)

    def test_is_fully_capturable_command_rejects_unmapped_return_value(self) -> None:
        # class_="sync" has no GLObjectKind/ObjectNameMapService support - stays unresolvable
        # regardless of command name (see _CLASS_TO_KIND's own comment).
        ret = ReturnValue(type='GLsync', class_='sync')
        self.assertFalse(_is_fully_capturable_command('glSomeHypotheticalCommand', ret, []))


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
        # glGenTransformFeedbacks/glCreateTransformFeedbacks/glDeleteTransformFeedbacks all
        # match the CREATE/DELETE naming heuristic perfectly, but transform feedback objects
        # have no GLObjectKind - confirms naming alone never overrides kind support.
        for cmd_name in (
            'glGenTransformFeedbacks', 'glCreateTransformFeedbacks',
            'glDeleteTransformFeedbacks',
        ):
            with self.subTest(command=cmd_name):
                cmd = get_command(Api.GL, cmd_name)
                self.assertFalse(cmd.enabled)


if __name__ == '__main__':
    unittest.main()
