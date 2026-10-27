# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import re
import sys
import unittest
from pathlib import Path

CODEGEN_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(CODEGEN_DIR))

from generator_helpers import object_name_override_for, wrapper_for
from intermediates import Api, Parameter, ReturnValue, TypeFacts
from intermediates_creator import (
    FILE_TO_API,
    _is_fully_capturable_command,  # pyright: ignore[reportPrivateUsage]
    _validate_object_name_override,  # pyright: ignore[reportPrivateUsage]
    extract_commands_from_registry,
    get_commands_from_xml_data,
)
from object_name_lifecycle import (
    GLObjectKind,
    ObjectNameOverride,
    ObjectNameRole,
    is_existence_query,
    resolve_object_name_override,
)
from registry import Registry

REGISTRY_PATHS: dict[Api, Path] = {
    api: path for path, api in FILE_TO_API.items()
}


def load_registry(api: Api) -> Registry:
    registry = Registry()
    registry.loadFile(REGISTRY_PATHS[api])
    return registry


class ValidateObjectNameOverrideTests(unittest.TestCase):
    def test_scalar_lookup_is_accepted(self) -> None:
        override = ObjectNameOverride(kind=GLObjectKind.TEXTURE, role=ObjectNameRole.LOOKUP)
        # Should not raise.
        _validate_object_name_override(
            'glBindTexture', 'texture', TypeFacts.from_type('GLuint'), override)

    def test_array_lookup_is_rejected(self) -> None:
        # No enabled command needs an array-of-existing-names lookup today - see
        # object_name_lifecycle.py's ObjectNameRole.LOOKUP docstring.
        override = ObjectNameOverride(kind=GLObjectKind.TEXTURE, role=ObjectNameRole.LOOKUP)
        with self.assertRaises(ValueError):
            _validate_object_name_override(
                'glSomeHypotheticalCommand', 'textures',
                TypeFacts.from_type('GLuint *'), override)

    def test_array_create_without_matching_pointer_override_is_rejected(self) -> None:
        # A command name/param name that has no POINTER_PARAM_OVERRIDES entry at all is a
        # broken table entry, not a silent-disable situation - mirrors
        # test_pointer_metadata.py's own "broken override raises" precedent.
        override = ObjectNameOverride(kind=GLObjectKind.TEXTURE, role=ObjectNameRole.CREATE)
        with self.assertRaises(ValueError):
            _validate_object_name_override(
                'glSomeHypotheticalCommand', 'names', TypeFacts.from_type('GLuint *'), override)

    def test_array_create_wrong_pointer_direction_is_rejected(self) -> None:
        # glDeleteTextures' 'textures' field really exists in POINTER_PARAM_OVERRIDES, but with
        # PointerDirection.IN - CREATE requires OUT, so pairing CREATE with this real IN entry
        # must still be rejected (this exercises the direction cross-check itself, not just
        # presence).
        override = ObjectNameOverride(kind=GLObjectKind.TEXTURE, role=ObjectNameRole.CREATE)
        with self.assertRaises(ValueError):
            _validate_object_name_override(
                'glDeleteTextures', 'textures', TypeFacts.from_type('const GLuint *'), override)

    def test_array_delete_with_matching_in_direction_is_accepted(self) -> None:
        override = ObjectNameOverride(kind=GLObjectKind.TEXTURE, role=ObjectNameRole.DELETE)
        # Should not raise: glDeleteTextures.textures really is a PointerDirection.IN entry.
        _validate_object_name_override(
            'glDeleteTextures', 'textures', TypeFacts.from_type('const GLuint *'), override)

    def test_neither_scalar_nor_array_field_is_rejected(self) -> None:
        # A double pointer is neither is_scalar (pointer_depth != 0) nor
        # is_array_element_eligible (that requires pointer_depth == 1 exactly) - a made-up
        # shape no real GL command uses, exercising the "field is neither" fallback branch.
        override = ObjectNameOverride(kind=GLObjectKind.PROGRAM, role=ObjectNameRole.LOOKUP)
        with self.assertRaises(ValueError):
            _validate_object_name_override(
                'glSomeHypotheticalCommand', 'names', TypeFacts.from_type('GLuint **'), override)

    def test_return_value_must_be_create(self) -> None:
        override = ObjectNameOverride(kind=GLObjectKind.PROGRAM, role=ObjectNameRole.LOOKUP)
        with self.assertRaises(ValueError):
            _validate_object_name_override(
                'glSomeHypotheticalCommand', 'return_value', TypeFacts.from_type('GLuint'),
                override)

    def test_return_value_create_is_accepted(self) -> None:
        override = ObjectNameOverride(kind=GLObjectKind.PROGRAM, role=ObjectNameRole.CREATE)
        # Should not raise.
        _validate_object_name_override(
            'glCreateProgram', 'return_value', TypeFacts.from_type('GLuint'), override)


class WrapperForTests(unittest.TestCase):
    """wrapper_for()'s object-name-override fork - see generator_helpers.py."""

    def test_is_commands_are_existence_queries(self) -> None:
        lookup = ObjectNameOverride(kind=GLObjectKind.TEXTURE, role=ObjectNameRole.LOOKUP)
        self.assertTrue(is_existence_query('glIsTexture', 'GLboolean', lookup))
        self.assertTrue(is_existence_query('glIsTextureEXT', 'GLboolean', lookup))
        self.assertFalse(is_existence_query('glBindTexture', 'void', lookup))
        self.assertFalse(is_existence_query('glIsEnabled', 'void', lookup))
        self.assertFalse(is_existence_query('glIsNamedBufferResidentNV', 'GLboolean', lookup))
        delete = ObjectNameOverride(kind=GLObjectKind.TEXTURE, role=ObjectNameRole.DELETE)
        self.assertFalse(is_existence_query('glIsTexture', 'GLboolean', delete))

    def test_scalar_with_no_override_is_plain_argument(self) -> None:
        self.assertEqual(wrapper_for(TypeFacts.from_type('GLuint'), None), 'Argument')

    def test_scalar_with_override_is_handle_argument(self) -> None:
        override = ObjectNameOverride(kind=GLObjectKind.TEXTURE, role=ObjectNameRole.LOOKUP)
        self.assertEqual(wrapper_for(TypeFacts.from_type('GLuint'), override), 'HandleArgument')

    def test_array_with_no_override_is_plain_array_argument(self) -> None:
        self.assertEqual(wrapper_for(TypeFacts.from_type('GLuint *'), None), 'ArrayArgument')

    def test_array_with_override_is_handle_array_argument(self) -> None:
        override = ObjectNameOverride(kind=GLObjectKind.TEXTURE, role=ObjectNameRole.CREATE)
        self.assertEqual(
            wrapper_for(TypeFacts.from_type('GLuint *'), override), 'HandleArrayArgument')

    def test_mapped_handle_is_unaffected_by_object_name_override_being_none(self) -> None:
        # HGLRC's own is_mapped_handle path must still work now that wrapper_for() takes a
        # second parameter - no OBJECT_NAME_PARAM_OVERRIDES entry ever applies to an HGLRC
        # field, so this should behave exactly as before this feature existed.
        self.assertEqual(wrapper_for(TypeFacts.from_type('HGLRC'), None), 'HandleArgument')


class ObjectNameOverrideForTests(unittest.TestCase):
    _raw_commands: dict[str, dict] | None = None

    def _raw_command(self, name: str) -> dict:
        if ObjectNameOverrideForTests._raw_commands is None:
            ObjectNameOverrideForTests._raw_commands = {
                c['command_attribs']['name']: c
                for c in extract_commands_from_registry(load_registry(Api.GL))}
        return ObjectNameOverrideForTests._raw_commands[name]

    def _assert_same_family_gen_and_is(
            self, bind_name: str, override: ObjectNameOverride, class_: str) -> None:
        # The replay-time name must come from the bind's own entry-point family, and the
        # recorder confirms creation with that family's Is command.
        suffix = re.search(r'(ARB|EXT|APPLE)$', bind_name)
        family = suffix.group(1) if suffix else ''
        for name in (override.gen_command, override.is_command):
            assert name is not None
            self.assertTrue(name.endswith(family) if family else not re.search(
                r'(ARB|EXT|APPLE)$', name), name)
        cmd = get_commands_from_xml_data(
            {Api.GL: [self._raw_command(override.gen_command)]})[Api.GL][0]
        self.assertEqual([p.type for p in cmd.params], ['GLsizei', 'GLuint *'])
        self.assertEqual(cmd.params[1].class_, class_)
        is_cmd = get_commands_from_xml_data(
            {Api.GL: [self._raw_command(override.is_command)]})[Api.GL][0]
        self.assertEqual(is_cmd.return_value.type.strip(), 'GLboolean')
        self.assertEqual([p.type for p in is_cmd.params], ['GLuint'])

    def test_implicit_create_fields_are_only_for_lookup_or_create(self) -> None:
        with self.assertRaises(ValueError):
            ObjectNameOverride(kind=GLObjectKind.TEXTURE, role=ObjectNameRole.LOOKUP_OR_CREATE)
        with self.assertRaises(ValueError):
            ObjectNameOverride(kind=GLObjectKind.TEXTURE, role=ObjectNameRole.LOOKUP,
                               gen_command='glGenTextures', is_command='glIsTexture')

    def test_known_entry_is_found(self) -> None:
        # glBindTexture and glBindBuffer are LOOKUP_OR_CREATE entries (see
        # test_lookup_or_create_entries_resolve below), so glBindBufferBase is the plain-LOOKUP
        # example here.
        override = object_name_override_for('glBindBufferBase', 'buffer', 'buffer')
        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.kind, GLObjectKind.BUFFER)
        self.assertEqual(override.role, ObjectNameRole.LOOKUP)

    def test_lookup_or_create_entries_resolve(self) -> None:
        # See _EXPLICIT_OVERRIDES' own comment for exactly why each of these seven (command,
        # field) pairs is a documented exception to plain auto-derived LOOKUP: each one's own
        # GL/extension spec text says binding a never-before-seen name implicitly creates a new
        # object with that name, unlike the later ARB-promoted core Bind commands for the
        # framebuffer, renderbuffer and vertex array namespaces
        # (glBindFramebuffer/glBindRenderbuffer/glBindVertexArray), which correctly keep plain
        # LOOKUP.
        # glBindVertexArrayOES is deliberately NOT here - see its own case in core_cases below
        # and _EXPLICIT_OVERRIDES' comment on why (gl.xml aliases it directly to core
        # glBindVertexArray, unlike its APPLE sibling here).
        cases = [
            ('glBindTexture', 'texture', 'texture', GLObjectKind.TEXTURE),
            ('glBindTextureEXT', 'texture', 'texture', GLObjectKind.TEXTURE),
            ('glBindBuffer', 'buffer', 'buffer', GLObjectKind.BUFFER),
            ('glBindBufferARB', 'buffer', 'buffer', GLObjectKind.BUFFER),
            ('glBindFramebufferEXT', 'framebuffer', 'framebuffer', GLObjectKind.FRAMEBUFFER),
            ('glBindRenderbufferEXT', 'renderbuffer', 'renderbuffer', GLObjectKind.RENDERBUFFER),
            ('glBindVertexArrayAPPLE', 'array', 'vertex array', GLObjectKind.VERTEX_ARRAY),
        ]
        for command_name, field_name, class_, kind in cases:
            with self.subTest(command=command_name):
                override = object_name_override_for(command_name, field_name, class_)
                self.assertIsNotNone(override)
                assert override is not None
                self.assertEqual(override.kind, kind)
                self.assertEqual(override.role, ObjectNameRole.LOOKUP_OR_CREATE)
                self._assert_same_family_gen_and_is(command_name, override, class_)

        # The later, ARB-promoted core Bind commands for the same three namespaces deliberately
        # keep plain, auto-derived LOOKUP - core FBO/renderbuffer/vertex-array spec text
        # requires a pre-existing name from the matching Gen call, unlike the EXT/APPLE
        # variants above. glBindVertexArrayOES joins this list (not the LOOKUP_OR_CREATE one
        # above) because gl.xml declares it a true <alias> of glBindVertexArray itself - Khronos's
        # own statement that the two are the identical command, not merely same-shaped ones.
        core_cases = [
            ('glBindFramebuffer', 'framebuffer', 'framebuffer', GLObjectKind.FRAMEBUFFER),
            ('glBindRenderbuffer', 'renderbuffer', 'renderbuffer', GLObjectKind.RENDERBUFFER),
            ('glBindVertexArray', 'array', 'vertex array', GLObjectKind.VERTEX_ARRAY),
            ('glBindVertexArrayOES', 'array', 'vertex array', GLObjectKind.VERTEX_ARRAY),
        ]
        for command_name, field_name, class_, kind in core_cases:
            with self.subTest(command=command_name):
                override = object_name_override_for(command_name, field_name, class_)
                self.assertIsNotNone(override)
                assert override is not None
                self.assertEqual(override.kind, kind)
                self.assertEqual(override.role, ObjectNameRole.LOOKUP)

    def test_unknown_entry_is_none(self) -> None:
        # A field with no class_ at all (a plain GLenum/index/whatever, not an object name)
        # always resolves to None, regardless of command/param name - mirrors
        # test_glbindattriblocation_index_param_is_not_remapped's own precedent.
        self.assertIsNone(object_name_override_for('glBindFramebuffer', 'target', None))
        self.assertIsNone(object_name_override_for('glBindVertexArray', 'nonexistent_param', None))

    def test_return_value_entry_is_found(self) -> None:
        override = object_name_override_for('glCreateProgram', 'return_value', 'program')
        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.kind, GLObjectKind.PROGRAM)
        self.assertEqual(override.role, ObjectNameRole.CREATE)


class ResolveObjectNameOverrideTests(unittest.TestCase):
    """object_name_lifecycle.py's own auto-derivation mechanism - class_ -> kind, command name
    -> role - independent of generator_helpers.py's thin object_name_override_for() wrapper
    tested above."""

    def test_no_class_is_none(self) -> None:
        self.assertIsNone(resolve_object_name_override('glBindBufferBase', 'index', None))

    def test_copy_image_names_take_their_kind_from_the_target(self) -> None:
        override = resolve_object_name_override('glCopyImageSubData', 'dstName', None)
        self.assertEqual(override, ObjectNameOverride(
            kind=GLObjectKind.TEXTURE, role=ObjectNameRole.LOOKUP, target_param='dstTarget'))

    def test_object_label_name_takes_its_kind_from_the_identifier(self) -> None:
        for cmd_name in ('glObjectLabel', 'glObjectLabelKHR'):
            with self.subTest(command=cmd_name):
                override = resolve_object_name_override(cmd_name, 'name', None)
                self.assertEqual(override, ObjectNameOverride(
                    kind=None, role=ObjectNameRole.LOOKUP, identifier_param='identifier'))

    def test_identifier_param_requires_no_kind(self) -> None:
        with self.assertRaises(ValueError):
            ObjectNameOverride(kind=GLObjectKind.BUFFER, role=ObjectNameRole.LOOKUP,
                               identifier_param='identifier')
        with self.assertRaises(ValueError):
            ObjectNameOverride(kind=None, role=ObjectNameRole.LOOKUP)

    def test_unsupported_kind_is_none(self) -> None:
        # 'sync' is a real gl.xml class= value this backend has no GLObjectKind/
        # ObjectNameMapService support for yet - see _CLASS_TO_KIND's own comment.
        self.assertIsNone(resolve_object_name_override('glDeleteSync', 'sync', 'sync'))

    def test_unknown_class_raises(self) -> None:
        # A class= string this module has never seen must fail loudly, not silently resolve to
        # None (which would be indistinguishable from "known but unsupported") - see
        # resolve_object_name_override's own docstring.
        with self.assertRaises(KeyError):
            resolve_object_name_override('glSomeHypotheticalCommand', 'thing', 'some new kind')

    def test_gen_and_create_prefixes_infer_create(self) -> None:
        for cmd_name in ('glGenTextures', 'glCreateTextures'):
            with self.subTest(command=cmd_name):
                override = resolve_object_name_override(cmd_name, 'textures', 'texture')
                assert override is not None
                self.assertEqual(override.role, ObjectNameRole.CREATE)

    def test_delete_prefix_infers_delete(self) -> None:
        override = resolve_object_name_override('glDeleteTextures', 'textures', 'texture')
        assert override is not None
        self.assertEqual(override.role, ObjectNameRole.DELETE)

    def test_everything_else_infers_lookup(self) -> None:
        # See ObjectNameOverrideForTests.test_known_entry_is_found for why this uses
        # glBindBufferBase.
        override = resolve_object_name_override('glBindBufferBase', 'buffer', 'buffer')
        assert override is not None
        self.assertEqual(override.role, ObjectNameRole.LOOKUP)

    def test_generate_is_not_a_create_false_positive(self) -> None:
        # glGenerateTextureMipmap/glGenerateMipmap regenerate mipmaps for an EXISTING texture -
        # a LOOKUP, not a CREATE - despite starting with the literal substring "glGen". This is
        # the exact naming trap _GEN_OR_CREATE_RE's lookahead exists to avoid; a naive
        # `startswith('glGen')` check would misclassify this as CREATE.
        override = resolve_object_name_override('glGenerateTextureMipmap', 'texture', 'texture')
        assert override is not None
        self.assertEqual(override.kind, GLObjectKind.TEXTURE)
        self.assertEqual(override.role, ObjectNameRole.LOOKUP)

    def test_explicit_override_wins_over_naming_heuristic(self) -> None:
        # glFreeObjectBufferATI genuinely deletes its buffer's name despite not matching
        # `glDelete*` - the one real exception found across the whole registry audit (see
        # _EXPLICIT_OVERRIDES's own comment). Naming alone would say LOOKUP; the explicit
        # override must take precedence.
        override = resolve_object_name_override('glFreeObjectBufferATI', 'buffer', 'buffer')
        assert override is not None
        self.assertEqual(override.kind, GLObjectKind.BUFFER)
        self.assertEqual(override.role, ObjectNameRole.DELETE)


class CrossNamespaceIndependenceTests(unittest.TestCase):
    """The one genuinely new risk this classification (and ObjectNameMapService) exists to
    avoid: two different namespaces sharing the same numeric GLuint value must classify
    independently."""

    def test_texture_and_buffer_overrides_are_independent_objects(self) -> None:
        texture_override = resolve_object_name_override('glGenTextures', 'textures', 'texture')
        buffer_override = resolve_object_name_override('glGenBuffers', 'buffers', 'buffer')
        assert texture_override is not None
        assert buffer_override is not None
        self.assertNotEqual(texture_override.kind, buffer_override.kind)
        self.assertEqual(texture_override.role, buffer_override.role)


class EndToEndClassificationTests(unittest.TestCase):
    """Confirms the real, enabled commands this task touches actually get the expected
    wrapper type from real registry data - mirrors test_pointer_metadata.py's
    EndToEndEnabledGateTests pattern."""

    def _get_command(self, api: Api, name: str):
        registry = load_registry(api)
        raw_commands = extract_commands_from_registry(registry)
        raw = next(c for c in raw_commands if c['command_attribs']['name'] == name)
        return get_commands_from_xml_data({api: [raw]})[api][0]

    def test_array_output_creation_commands(self) -> None:
        for cmd_name, param_name in (('glGenTextures', 'textures'), ('glGenBuffers', 'buffers')):
            with self.subTest(command=cmd_name):
                cmd = self._get_command(Api.GL, cmd_name)
                self.assertTrue(cmd.enabled)
                param = next(p for p in cmd.params if p.name == param_name)
                override = object_name_override_for(cmd_name, param_name, param.class_)
                self.assertEqual(wrapper_for(param.type_facts, override), 'HandleArrayArgument')

    def test_array_input_destruction_command(self) -> None:
        cmd = self._get_command(Api.GL, 'glDeleteTextures')
        self.assertTrue(cmd.enabled)
        param = next(p for p in cmd.params if p.name == 'textures')
        override = object_name_override_for('glDeleteTextures', 'textures', param.class_)
        self.assertEqual(wrapper_for(param.type_facts, override), 'HandleArrayArgument')

    def test_scalar_return_creation_commands(self) -> None:
        for cmd_name in ('glCreateProgram', 'glCreateShader'):
            with self.subTest(command=cmd_name):
                cmd = self._get_command(Api.GL, cmd_name)
                self.assertTrue(cmd.enabled)
                override = object_name_override_for(
                    cmd_name, 'return_value', cmd.return_value.class_)
                self.assertEqual(
                    wrapper_for(cmd.return_value.type_facts, override), 'HandleArgument')

    def test_scalar_destruction_commands(self) -> None:
        for cmd_name, param_name in (
            ('glDeleteProgram', 'program'), ('glDeleteShader', 'shader'),
        ):
            with self.subTest(command=cmd_name):
                cmd = self._get_command(Api.GL, cmd_name)
                self.assertTrue(cmd.enabled)
                param = next(p for p in cmd.params if p.name == param_name)
                override = object_name_override_for(cmd_name, param_name, param.class_)
                self.assertEqual(wrapper_for(param.type_facts, override), 'HandleArgument')

    def test_scalar_lookup_commands(self) -> None:
        for cmd_name, param_name in (
            ('glBindTexture', 'texture'),
            ('glBindBuffer', 'buffer'),
            ('glUseProgram', 'program'),
            ('glBindAttribLocation', 'program'),
            ('glGetUniformLocation', 'program'),
            # Rendering-pipeline round 2 additions (see
            # docs/development/opengl-rendering-pipeline-pointer-support-plan.md) - closes
            # the shader pipeline's own previously-unmapped GLuint params.
            ('glShaderSource', 'shader'),
            ('glCompileShader', 'shader'),
            ('glAttachShader', 'program'),
            ('glAttachShader', 'shader'),
            ('glLinkProgram', 'program'),
        ):
            with self.subTest(command=cmd_name):
                cmd = self._get_command(Api.GL, cmd_name)
                self.assertTrue(cmd.enabled)
                param = next(p for p in cmd.params if p.name == param_name)
                override = object_name_override_for(cmd_name, param_name, param.class_)
                self.assertEqual(wrapper_for(param.type_facts, override), 'HandleArgument')

    def test_glbindattriblocation_index_param_is_not_remapped(self) -> None:
        # 'index' is a plain vertex-attribute index, not an object name - gl.xml never tags it
        # with class=, so it has no class_ to auto-derive from.
        cmd = self._get_command(Api.GL, 'glBindAttribLocation')
        param = next(p for p in cmd.params if p.name == 'index')
        self.assertIsNone(param.class_)
        self.assertIsNone(object_name_override_for('glBindAttribLocation', 'index', param.class_))
        self.assertEqual(wrapper_for(param.type_facts, None), 'Argument')

    def test_framebuffer_and_vertex_array_bind_commands_are_remapped(self) -> None:
        # FBO-pipeline round (see docs/development/opengl-player-skeleton-plan.md's
        # Decision 10): glGenFramebuffers/glGenVertexArrays are now enabled, so
        # glBindFramebuffer/glBindVertexArray's object-name params are real LOOKUPs, not the
        # reserved-default-object-0-only case this test used to cover.
        for cmd_name, param_name, kind in (
            ('glBindFramebuffer', 'framebuffer', GLObjectKind.FRAMEBUFFER),
            ('glBindVertexArray', 'array', GLObjectKind.VERTEX_ARRAY),
        ):
            with self.subTest(command=cmd_name):
                cmd = self._get_command(Api.GL, cmd_name)
                self.assertTrue(cmd.enabled)
                param = next(p for p in cmd.params if p.name == param_name)
                override = object_name_override_for(cmd_name, param_name, param.class_)
                self.assertIsNotNone(override)
                assert override is not None
                self.assertEqual(override.kind, kind)
                self.assertEqual(override.role, ObjectNameRole.LOOKUP)
                self.assertEqual(wrapper_for(param.type_facts, override), 'HandleArgument')

    def test_query_gen_delete_commands_are_remapped(self) -> None:
        # Query-object round (see GLObjectKind.QUERY's own comment) - same CREATE/DELETE array
        # shape as glGenTextures/glDeleteTextures above, now for GL_QUERY names.
        for cmd_name, param_name, role in (
            ('glGenQueries', 'ids', ObjectNameRole.CREATE),
            ('glDeleteQueries', 'ids', ObjectNameRole.DELETE),
        ):
            with self.subTest(command=cmd_name):
                cmd = self._get_command(Api.GL, cmd_name)
                self.assertTrue(cmd.enabled)
                param = next(p for p in cmd.params if p.name == param_name)
                override = object_name_override_for(cmd_name, param_name, param.class_)
                self.assertIsNotNone(override)
                assert override is not None
                self.assertEqual(override.kind, GLObjectKind.QUERY)
                self.assertEqual(override.role, role)
                self.assertEqual(wrapper_for(param.type_facts, override), 'HandleArrayArgument')

    def test_query_id_params_create_unused_names(self) -> None:
        # Plain scalar params, no pointer to gate on. An unused id creates the query, so each is
        # LOOKUP_OR_CREATE with its own family's Gen/Is commands.
        cases = {
            'glBeginQuery': ('glGenQueries', 'glIsQuery'),
            'glBeginQueryARB': ('glGenQueriesARB', 'glIsQueryARB'),
            'glBeginQueryIndexed': ('glGenQueries', 'glIsQuery'),
            'glQueryCounter': ('glGenQueries', 'glIsQuery'),
        }
        for name, (gen_command, is_command) in cases.items():
            with self.subTest(command=name):
                cmd = self._get_command(Api.GL, name)
                self.assertTrue(cmd.enabled)
                param = next(p for p in cmd.params if p.name == 'id')
                override = object_name_override_for(name, 'id', param.class_)
                self.assertIsNotNone(override)
                assert override is not None
                self.assertEqual(override.kind, GLObjectKind.QUERY)
                self.assertEqual(override.role, ObjectNameRole.LOOKUP_OR_CREATE)
                self.assertEqual(override.gen_command, gen_command)
                self.assertEqual(override.is_command, is_command)
                self.assertEqual(wrapper_for(param.type_facts, override), 'HandleArgument')

    def test_renderbuffer_sampler_program_pipeline_gen_delete_commands_are_remapped(self) -> None:
        # Renderbuffer/Sampler/ProgramPipeline (see
        # GLObjectKind.RENDERBUFFER/SAMPLER/PROGRAM_PIPELINE's own comments): same CREATE/
        # DELETE array shape as glGenQueries/glDeleteQueries above, now for these three kinds.
        # glGenSamplers/glDeleteSamplers' sibling count param is named `count`, not `n` - see
        # pointer_metadata.py's own comment on this pair.
        for cmd_name, param_name, kind, role in (
            ('glGenRenderbuffers', 'renderbuffers', GLObjectKind.RENDERBUFFER,
             ObjectNameRole.CREATE),
            ('glDeleteRenderbuffers', 'renderbuffers', GLObjectKind.RENDERBUFFER,
             ObjectNameRole.DELETE),
            ('glGenSamplers', 'samplers', GLObjectKind.SAMPLER, ObjectNameRole.CREATE),
            ('glDeleteSamplers', 'samplers', GLObjectKind.SAMPLER, ObjectNameRole.DELETE),
            ('glGenProgramPipelines', 'pipelines', GLObjectKind.PROGRAM_PIPELINE,
             ObjectNameRole.CREATE),
            ('glDeleteProgramPipelines', 'pipelines', GLObjectKind.PROGRAM_PIPELINE,
             ObjectNameRole.DELETE),
        ):
            with self.subTest(command=cmd_name):
                cmd = self._get_command(Api.GL, cmd_name)
                self.assertTrue(cmd.enabled)
                param = next(p for p in cmd.params if p.name == param_name)
                override = object_name_override_for(cmd_name, param_name, param.class_)
                self.assertIsNotNone(override)
                assert override is not None
                self.assertEqual(override.kind, kind)
                self.assertEqual(override.role, role)
                self.assertEqual(wrapper_for(param.type_facts, override), 'HandleArrayArgument')

    def test_renderbuffer_sampler_program_pipeline_bind_commands_are_remapped(self) -> None:
        # glBindRenderbuffer/glBindSampler/glBindProgramPipeline - each was already
        # `enabled=True` (scalar-only, no pointer to gate on) with no lifecycle tracking for
        # its own object namespace at all.
        for cmd_name, param_name, kind in (
            ('glBindRenderbuffer', 'renderbuffer', GLObjectKind.RENDERBUFFER),
            ('glBindSampler', 'sampler', GLObjectKind.SAMPLER),
            ('glBindProgramPipeline', 'pipeline', GLObjectKind.PROGRAM_PIPELINE),
        ):
            with self.subTest(command=cmd_name):
                cmd = self._get_command(Api.GL, cmd_name)
                self.assertTrue(cmd.enabled)
                param = next(p for p in cmd.params if p.name == param_name)
                override = object_name_override_for(cmd_name, param_name, param.class_)
                self.assertIsNotNone(override)
                assert override is not None
                self.assertEqual(override.kind, kind)
                self.assertEqual(override.role, ObjectNameRole.LOOKUP)
                self.assertEqual(wrapper_for(param.type_facts, override), 'HandleArgument')

    def test_glbindsampler_unit_param_is_not_remapped(self) -> None:
        # glBindSampler's first param, 'unit', is a plain texture-unit index, not an object
        # name - gl.xml never tags it with class=, mirroring
        # test_glbindattriblocation_index_param_is_not_remapped's own shape above.
        cmd = self._get_command(Api.GL, 'glBindSampler')
        param = next(p for p in cmd.params if p.name == 'unit')
        self.assertIsNone(param.class_)
        self.assertIsNone(object_name_override_for('glBindSampler', 'unit', param.class_))
        self.assertEqual(wrapper_for(param.type_facts, None), 'Argument')

    def test_glfreeobjectbufferati_is_remapped_as_delete(self) -> None:
        # The one real _EXPLICIT_OVERRIDES exception (see that table's own comment):
        # glFreeObjectBufferATI's name does not match `glDelete*`, but it genuinely deletes its
        # buffer's name - confirms the explicit override reaches real registry data end to end,
        # not just the unit-level check in ResolveObjectNameOverrideTests above.
        cmd = self._get_command(Api.GL, 'glFreeObjectBufferATI')
        self.assertTrue(cmd.enabled)
        param = next(p for p in cmd.params if p.name == 'buffer')
        self.assertEqual(param.class_, 'buffer')
        override = object_name_override_for('glFreeObjectBufferATI', 'buffer', param.class_)
        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.kind, GLObjectKind.BUFFER)
        self.assertEqual(override.role, ObjectNameRole.DELETE)
        self.assertEqual(wrapper_for(param.type_facts, override), 'HandleArgument')

    def test_deletetransformfeedbacks_is_remapped_as_delete(self) -> None:
        # Confirms glDeleteTransformFeedbacks' `ids` array is genuinely classified as a
        # DELETE-role TransformFeedback field - the precondition for
        # ObjectNameMapServiceDeleteCppSourceTests below
        # to matter at all: if this were somehow unmapped, TransformFeedback name deletion
        # would never call ObjectNameMapService::Delete() in the first place.
        cmd = self._get_command(Api.GL, 'glDeleteTransformFeedbacks')
        self.assertTrue(cmd.enabled)
        param = next(p for p in cmd.params if p.name == 'ids')
        override = object_name_override_for('glDeleteTransformFeedbacks', 'ids', param.class_)
        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.kind, GLObjectKind.TRANSFORM_FEEDBACK)
        self.assertEqual(override.role, ObjectNameRole.DELETE)

    def test_query_kind_is_classified_alongside_transform_feedback(self) -> None:
        # object_name_lifecycle.py's own GLObjectKind classification must agree with the C++
        # GroupIdentityFor() bucketing GroupIdentityForCppSourceTests below verifies - both
        # must treat Query the same way they already treat TransformFeedback.
        cmd = self._get_command(Api.GL, 'glGenQueries')
        self.assertTrue(cmd.enabled)
        param = next(p for p in cmd.params if p.name == 'ids')
        override = object_name_override_for('glGenQueries', 'ids', param.class_)
        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.kind, GLObjectKind.QUERY)


class ShaderObjectsARBTests(unittest.TestCase):
    """GL_ARB_shader_objects' GLhandleARB fields carry no class= in gl.xml; they take their
    alias's class, or HANDLE_ARB_CLASSES'."""

    @classmethod
    def setUpClass(cls) -> None:
        raw_commands = extract_commands_from_registry(load_registry(Api.GL))
        commands = get_commands_from_xml_data({Api.GL: raw_commands})[Api.GL]
        cls.commands = {command.name: command for command in commands}

    def _override(self, cmd_name: str, param_name: str) -> ObjectNameOverride | None:
        cmd = self.commands[cmd_name]
        field = (cmd.return_value if param_name == 'return_value'
                 else next(p for p in cmd.params if p.name == param_name))
        return object_name_override_for(cmd_name, param_name, field.class_)

    def test_commands_are_enabled(self) -> None:
        for name in (
            'glAttachObjectARB', 'glBindAttribLocationARB', 'glCompileShaderARB',
            'glCreateProgramObjectARB', 'glCreateShaderObjectARB', 'glDeleteObjectARB',
            'glDetachObjectARB', 'glGetActiveAttribARB', 'glGetActiveUniformARB',
            'glGetAttribLocationARB', 'glGetHandleARB', 'glGetInfoLogARB',
            'glGetObjectParameterfvARB', 'glGetObjectParameterivARB', 'glGetShaderSourceARB',
            'glGetUniformLocationARB', 'glGetUniformfvARB', 'glGetUniformivARB',
            'glLinkProgramARB', 'glShaderSourceARB', 'glUseProgramObjectARB',
            'glValidateProgramARB',
        ):
            with self.subTest(command=name):
                self.assertTrue(self.commands[name].enabled)

    def test_attached_objects_query_stays_disabled(self) -> None:
        # Like glGetAttachedShaders: its `obj` output array has no class.
        self.assertFalse(self.commands['glGetAttachedObjectsARB'].enabled)

    def test_classes_are_inherited_from_the_alias(self) -> None:
        for cmd_name, param_name, kind, role in (
            ('glCreateShaderObjectARB', 'return_value', GLObjectKind.SHADER, ObjectNameRole.CREATE),
            ('glCreateProgramObjectARB', 'return_value', GLObjectKind.PROGRAM,
             ObjectNameRole.CREATE),
            ('glAttachObjectARB', 'containerObj', GLObjectKind.PROGRAM, ObjectNameRole.LOOKUP),
            ('glAttachObjectARB', 'obj', GLObjectKind.SHADER, ObjectNameRole.LOOKUP),
            ('glShaderSourceARB', 'shaderObj', GLObjectKind.SHADER, ObjectNameRole.LOOKUP),
            ('glUseProgramObjectARB', 'programObj', GLObjectKind.PROGRAM, ObjectNameRole.LOOKUP),
            ('glGetUniformfvARB', 'programObj', GLObjectKind.PROGRAM, ObjectNameRole.LOOKUP),
        ):
            with self.subTest(command=cmd_name, param=param_name):
                self.assertEqual(self._override(cmd_name, param_name),
                                 ObjectNameOverride(kind=kind, role=role))

    def test_unaliased_commands_use_the_explicit_table(self) -> None:
        self.assertEqual(self._override('glDeleteObjectARB', 'obj'),
                         ObjectNameOverride(kind=GLObjectKind.PROGRAM, role=ObjectNameRole.DELETE))
        for cmd_name in ('glGetInfoLogARB', 'glGetObjectParameterivARB'):
            with self.subTest(command=cmd_name):
                self.assertEqual(
                    self._override(cmd_name, 'obj'),
                    ObjectNameOverride(kind=GLObjectKind.PROGRAM, role=ObjectNameRole.LOOKUP))

    def test_current_program_query_result_is_not_mapped(self) -> None:
        self.assertIsNone(self._override('glGetHandleARB', 'return_value'))

    def test_unclassified_handle_disables_the_command(self) -> None:
        params = [Parameter(name='obj', type='GLhandleARB')]
        self.assertFalse(_is_fully_capturable_command(
            'glHypotheticalARB', ReturnValue(type='void'), params))

    def test_shader_and_program_share_a_bucket(self) -> None:
        source = (CODEGEN_DIR.parent / 'common' / 'services' /
                  'objectNameMapService.cpp').read_text(encoding='utf-8')
        match = re.search(r'ObjectNameMapService::BucketFor\(GLObjectKind kind\) \{(.*?)\n\}',
                          source, re.DOTALL)
        self.assertIsNotNone(match, 'ObjectNameMapService::BucketFor() definition not found')
        assert match is not None
        self.assertRegex(match.group(1), r'if \(kind == GLObjectKind::Shader\) \{\s*'
                                         r'kind = GLObjectKind::Program;')


class ObjectNameMapServiceDeleteCppSourceTests(unittest.TestCase):
    """Source-text regression coverage:
    ObjectNameMapService::Delete() must not erase a TransformFeedback name's forward mapping,
    for the same "do not resolve a still-legal reference to key 0" reason already established
    for Program/Shader - see that method's own header comment
    (common/services/objectNameMapService.h). There is no C++ unit test infrastructure in this
    repo (see AGENTS.md's hard build/test constraints), so this parses the actual hand-written
    C++ text the same way test_tex_sampler_parameter_metadata.py's CROP_RECT_OES regression
    test already does."""

    _OBJECT_NAME_MAP_SERVICE_CPP = (
        CODEGEN_DIR.parent / 'common' / 'services' / 'objectNameMapService.cpp')

    def test_delete_keeps_transform_feedback_mapping_alive_alongside_program_and_shader(
            self) -> None:
        source = self._OBJECT_NAME_MAP_SERVICE_CPP.read_text(encoding='utf-8')
        match = re.search(r'void ObjectNameMapService::Delete\((.*?)\n\}', source, re.DOTALL)
        self.assertIsNotNone(match, 'ObjectNameMapService::Delete() definition not found')
        assert match is not None
        body = match.group(1)
        # The early-return special case must name all three kinds together, then return
        # before ever reaching the unconditional NameToKey.erase() below it.
        condition_match = re.search(
            r'if \((.*?GLObjectKind::Program.*?)\)\s*\{\s*return;\s*\}', body, re.DOTALL)
        self.assertIsNotNone(
            condition_match, 'No early-return special-case "if (...) { return; }" found in '
            'ObjectNameMapService::Delete()')
        assert condition_match is not None
        condition = condition_match.group(1)
        self.assertIn('GLObjectKind::Program', condition)
        self.assertIn('GLObjectKind::Shader', condition)
        self.assertIn('GLObjectKind::TransformFeedback', condition)
        erase_pos = body.find('NameToKey.erase(')
        self.assertNotEqual(erase_pos, -1, 'NameToKey.erase() call not found')
        self.assertLess(condition_match.end(), erase_pos,
                        'The Program/Shader/TransformFeedback early return must run before '
                        'the unconditional NameToKey.erase() call')


class GroupIdentityForCppSourceTests(unittest.TestCase):
    """Source-text regression coverage: GroupIdentityFor() (recorder/include/
    handleArgumentUpdaters.h) must bucket GLObjectKind::Query by per-context identity, not by
    share group - per the GL spec's "Shared Objects and Multiple Contexts" rule, query objects
    are container objects never shared between contexts, exactly like Framebuffer/VertexArray/
    ProgramPipeline/TransformFeedback. There is no C++ unit test infrastructure in this repo
    (see AGENTS.md's hard build/test constraints), so this parses the actual hand-written C++
    text the same way ObjectNameMapServiceDeleteCppSourceTests above does."""

    _HANDLE_ARGUMENT_UPDATERS_H = (
        CODEGEN_DIR.parent / 'recorder' / 'include' / 'handleArgumentUpdaters.h')

    def test_group_identity_for_buckets_query_per_context(self) -> None:
        # The per-context-bucket condition itself now lives in IsPerContextObjectKind() (pulled
        # out of GroupIdentityFor's own body so ConfirmImplicitCreate's identity-mismatch sanity
        # check - see that function's own comment, handleArgumentUpdaters.h - can name the same
        # classification without duplicating the condition); GroupIdentityFor() itself now just
        # calls it. Assert both: the condition itself (still exactly this GL-spec-derived kind
        # list, just relocated) and that GroupIdentityFor() actually delegates to it rather than
        # having drifted back out of sync with its own helper.
        source = self._HANDLE_ARGUMENT_UPDATERS_H.read_text(encoding='utf-8')
        helper_match = re.search(
            r'inline bool IsPerContextObjectKind\(GLObjectKind kind\) \{(.*?)\n\}',
            source, re.DOTALL)
        self.assertIsNotNone(helper_match, 'IsPerContextObjectKind() definition not found')
        assert helper_match is not None
        helper_body = helper_match.group(1)
        condition_match = re.search(
            r'return (.*?GLObjectKind::Framebuffer.*?);', helper_body, re.DOTALL)
        self.assertIsNotNone(
            condition_match,
            'No per-context-bucket "kind == GLObjectKind::Framebuffer || ..." condition found '
            'in IsPerContextObjectKind()')
        assert condition_match is not None
        condition = condition_match.group(1)
        self.assertIn('GLObjectKind::Framebuffer', condition)
        self.assertIn('GLObjectKind::VertexArray', condition)
        self.assertIn('GLObjectKind::ProgramPipeline', condition)
        self.assertIn('GLObjectKind::TransformFeedback', condition)
        self.assertIn('GLObjectKind::Query', condition,
                      'GLObjectKind::Query must be bucketed per-context alongside '
                      'Framebuffer/VertexArray/ProgramPipeline/TransformFeedback - query '
                      'objects are container objects never shared between contexts per the GL '
                      'spec\'s "Shared Objects and Multiple Contexts" rule')

        group_identity_for_match = re.search(
            r'inline uint64_t GroupIdentityFor\(GLObjectKind kind\) \{(.*?)\n\}',
            source, re.DOTALL)
        self.assertIsNotNone(group_identity_for_match, 'GroupIdentityFor() definition not found')
        assert group_identity_for_match is not None
        self.assertIn('IsPerContextObjectKind(kind)', group_identity_for_match.group(1),
                      'GroupIdentityFor() must delegate to IsPerContextObjectKind() rather than '
                      're-deriving (and risking drifting out of sync with) the same condition')


if __name__ == '__main__':
    unittest.main()
