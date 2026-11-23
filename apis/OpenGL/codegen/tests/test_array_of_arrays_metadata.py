# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import sys
import unittest
from pathlib import Path

CODEGEN_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(CODEGEN_DIR))

from array_of_arrays_metadata import (
    ARRAY_OF_ARRAYS_PARAM_OVERRIDES,
    ArrayOfArraysOverride,
    array_of_arrays_override_for,
    consumed_param_owner,
    consumed_pointer_params,
)
from generator_helpers import member_cpp_type, object_name_override_for, wrapper_for
from intermediates import Api, Parameter, TypeFacts
from intermediates_creator import (
    FILE_TO_API,
    _validate_array_of_arrays_override,  # pyright: ignore[reportPrivateUsage]
    extract_commands_from_registry,
    get_commands_from_xml_data,
)
from registry import Registry

REGISTRY_PATHS: dict[Api, Path] = {
    api: path for path, api in FILE_TO_API.items()
}


def load_registry(api: Api) -> Registry:
    registry = Registry()
    registry.loadFile(REGISTRY_PATHS[api])
    return registry


class ValidateArrayOfArraysOverrideTests(unittest.TestCase):
    def test_missing_outer_count_param_raises(self) -> None:
        param = Parameter(name='string', type='const GLchar *const *')
        length = Parameter(name='length', type='const GLint *')
        override = ArrayOfArraysOverride(
            outer_count_param='count', element_length_param='length')

        with self.assertRaises(ValueError):
            _validate_array_of_arrays_override(
                'glShaderSource', param, override, other_params=[length])

    def test_non_scalar_outer_count_param_raises(self) -> None:
        param = Parameter(name='string', type='const GLchar *const *')
        count = Parameter(name='count', type='GLsizei *')  # not scalar
        length = Parameter(name='length', type='const GLint *')
        override = ArrayOfArraysOverride(
            outer_count_param='count', element_length_param='length')

        with self.assertRaises(ValueError):
            _validate_array_of_arrays_override(
                'glShaderSource', param, override, other_params=[count, length])

    def test_missing_element_length_param_raises(self) -> None:
        param = Parameter(name='string', type='const GLchar *const *')
        count = Parameter(name='count', type='GLsizei')
        override = ArrayOfArraysOverride(
            outer_count_param='count', element_length_param='length')

        with self.assertRaises(ValueError):
            _validate_array_of_arrays_override(
                'glShaderSource', param, override, other_params=[count])

    def test_non_pointer_element_length_param_raises(self) -> None:
        param = Parameter(name='string', type='const GLchar *const *')
        count = Parameter(name='count', type='GLsizei')
        length = Parameter(name='length', type='GLint')  # not a pointer
        override = ArrayOfArraysOverride(
            outer_count_param='count', element_length_param='length')

        with self.assertRaises(ValueError):
            _validate_array_of_arrays_override(
                'glShaderSource', param, override, other_params=[count, length])

    def test_valid_shape_is_accepted(self) -> None:
        param = Parameter(name='string', type='const GLchar *const *')
        count = Parameter(name='count', type='GLsizei')
        length = Parameter(name='length', type='const GLint *')
        override = ArrayOfArraysOverride(
            outer_count_param='count', element_length_param='length')

        # Should not raise.
        _validate_array_of_arrays_override(
            'glShaderSource', param, override, other_params=[count, length])


class ConsumedPointerParamsTests(unittest.TestCase):
    def test_element_length_param_is_consumed(self) -> None:
        self.assertIn('length', consumed_pointer_params('glShaderSource'))

    def test_outer_count_param_is_not_consumed(self) -> None:
        self.assertNotIn('count', consumed_pointer_params('glShaderSource'))

    def test_array_of_arrays_param_itself_is_not_consumed(self) -> None:
        self.assertNotIn('string', consumed_pointer_params('glShaderSource'))

    def test_unrelated_command_has_no_consumed_params(self) -> None:
        self.assertEqual(consumed_pointer_params('glDrawElements'), frozenset())


class ConsumedParamOwnerTests(unittest.TestCase):
    def test_known_consumed_param_resolves_to_owner(self) -> None:
        self.assertEqual(consumed_param_owner('glShaderSource', 'length'), 'string')

    def test_unconsumed_param_is_none(self) -> None:
        self.assertIsNone(consumed_param_owner('glShaderSource', 'count'))
        self.assertIsNone(consumed_param_owner('glShaderSource', 'string'))

    def test_unrelated_command_is_none(self) -> None:
        self.assertIsNone(consumed_param_owner('glDrawElements', 'indices'))


class ArrayOfArraysOverrideForTests(unittest.TestCase):
    def test_known_entry_is_found(self) -> None:
        override = array_of_arrays_override_for('glShaderSource', 'string')
        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.outer_count_param, 'count')
        self.assertEqual(override.element_length_param, 'length')

    def test_unknown_entry_is_none(self) -> None:
        self.assertIsNone(array_of_arrays_override_for('glDrawElements', 'indices'))


class WrapperForAndMemberCppTypeTests(unittest.TestCase):
    """wrapper_for()/member_cpp_type()'s array-of-arrays fork - see generator_helpers.py."""

    def test_array_of_arrays_override_selects_array_of_arrays_wrapper(self) -> None:
        override = ARRAY_OF_ARRAYS_PARAM_OVERRIDES[('glShaderSource', 'string')]
        self.assertEqual(
            wrapper_for(TypeFacts.from_type('const GLchar *const *'), None, None, override),
            'ArrayOfArrays')

    def test_array_of_arrays_member_cpp_type_is_element_base_type(self) -> None:
        override = ARRAY_OF_ARRAYS_PARAM_OVERRIDES[('glShaderSource', 'string')]
        param = Parameter(name='string', type='const GLchar *const *')
        self.assertEqual(member_cpp_type(param, None, override), 'GLchar')

    def test_mapped_handle_member_cpp_type_drops_top_level_const(self) -> None:
        param = Parameter(name='context', type='const GLXContext')
        self.assertEqual(member_cpp_type(param), 'GLXContext')
        self.assertEqual(member_cpp_type(Parameter(name='vis', type='XVisualInfo *')),
                         'XVisualInfo *')

    def test_no_override_is_unaffected_by_new_parameters(self) -> None:
        # wrapper_for()'s pre-existing callers (three positional args) must behave exactly as
        # before now that it takes two more optional parameters.
        self.assertEqual(wrapper_for(TypeFacts.from_type('GLuint'), None), 'Argument')


class EndToEndClassificationTests(unittest.TestCase):
    """Confirms glShaderSource actually becomes enabled=True with the expected wrapper shapes,
    from real registry data - mirrors test_pointer_metadata.py's EndToEndEnabledGateTests."""

    def _get_command(self, api: Api, name: str):
        registry = load_registry(api)
        raw_commands = extract_commands_from_registry(registry)
        raw = next(c for c in raw_commands if c['command_attribs']['name'] == name)
        return get_commands_from_xml_data({api: [raw]})[api][0]

    def test_glshadersource_is_enabled(self) -> None:
        cmd = self._get_command(Api.GL, 'glShaderSource')
        self.assertTrue(cmd.enabled)

    def test_glshadersource_string_is_array_of_arrays(self) -> None:
        cmd = self._get_command(Api.GL, 'glShaderSource')
        param = next(p for p in cmd.params if p.name == 'string')
        override = array_of_arrays_override_for('glShaderSource', 'string')
        self.assertEqual(wrapper_for(param.type_facts, None, None, override), 'ArrayOfArrays')

    def test_glshadersource_shader_reuses_object_name_mapping(self) -> None:
        # See docs/development/opengl-rendering-pipeline-pointer-support-plan.md: `shader`
        # reuses the pre-existing HandleArgument<GLuint> mechanism, unrelated to `string`'s
        # own ArrayOfArrays classification.
        cmd = self._get_command(Api.GL, 'glShaderSource')
        param = next(p for p in cmd.params if p.name == 'shader')
        override = object_name_override_for('glShaderSource', 'shader', param.class_)
        self.assertIsNotNone(override)
        self.assertEqual(wrapper_for(param.type_facts, override), 'HandleArgument')

    def test_glshadersource_length_has_no_member_of_its_own(self) -> None:
        self.assertIn('length', consumed_pointer_params('glShaderSource'))

    def test_shader_pipeline_commands_map_their_gluint_params(self) -> None:
        # glCompileShader/glAttachShader/glLinkProgram were already enabled but had no
        # object_name_lifecycle.py entry previously - see that module's comment.
        for cmd_name, param_name in (
            ('glCompileShader', 'shader'),
            ('glAttachShader', 'program'),
            ('glAttachShader', 'shader'),
            ('glLinkProgram', 'program'),
        ):
            with self.subTest(command=cmd_name, param=param_name):
                cmd = self._get_command(Api.GL, cmd_name)
                self.assertTrue(cmd.enabled)
                param = next(p for p in cmd.params if p.name == param_name)
                override = object_name_override_for(cmd_name, param_name, param.class_)
                self.assertIsNotNone(override)
                self.assertEqual(wrapper_for(param.type_facts, override), 'HandleArgument')


if __name__ == '__main__':
    unittest.main()
