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

from generator_helpers import pointer_override_for
from intermediates import Api, Command, CmdType, Parameter, ReturnValue
from intermediates_creator import (
    FILE_TO_API,
    _is_param_capturable,  # pyright: ignore[reportPrivateUsage]
    extract_commands_from_registry,
    get_commands_from_xml_data,
)
from pointer_metadata import LengthSourceKind, PointerDirection, POINTER_PARAM_OVERRIDES
from registry import Registry
from uniform_setter_metadata import derive_uniform_setter_override

REGISTRY_PATHS: dict[Api, Path] = {
    api: path for path, api in FILE_TO_API.items()
}


def load_registry(api: Api) -> Registry:
    registry = Registry()
    registry.loadFile(REGISTRY_PATHS[api])
    return registry


def _location_param() -> Parameter:
    return Parameter(name='location', type='GLint')


def _program_param() -> Parameter:
    return Parameter(name='program', type='GLuint')


def _count_param() -> Parameter:
    return Parameter(name='count', type='GLsizei')


def _transpose_param() -> Parameter:
    return Parameter(name='transpose', type='GLboolean')


class DeriveUniformSetterOverrideVectorUnitTests(unittest.TestCase):
    """Direct unit tests against derive_uniform_setter_override() for the vector
    (glUniform*v/glProgramUniform*v) family - no registry I/O, so every accept/reject boundary
    from that function's own header comment is pinned down precisely."""

    def test_uniform1dv_is_derived_with_multiplier_one(self) -> None:
        # glUniform1dv's own shape: len="count" (no '*N' - gl.xml canonicalizes N=1 to a plain
        # PARAM_NAME, not SCALED_PARAM_NAME) - still must derive multiplier=1.
        param = Parameter(name='value', type='const GLdouble *', len='count')
        other = [_location_param(), _count_param()]

        override = derive_uniform_setter_override('glUniform1dv', param, other)

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.direction, PointerDirection.IN)
        self.assertEqual(override.length_source_kind, LengthSourceKind.SIBLING_PARAM)
        self.assertEqual(override.length_source_value, 'count')
        self.assertEqual(override.length_multiplier, 1)

    def test_uniform2dv_is_derived_with_multiplier_two(self) -> None:
        param = Parameter(name='value', type='const GLdouble *', len='count*2')
        other = [_location_param(), _count_param()]

        override = derive_uniform_setter_override('glUniform2dv', param, other)

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.length_multiplier, 2)

    def test_uniform3dv_is_derived_with_multiplier_three(self) -> None:
        param = Parameter(name='value', type='const GLdouble *', len='count*3')
        other = [_location_param(), _count_param()]

        override = derive_uniform_setter_override('glUniform3dv', param, other)

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.length_multiplier, 3)

    def test_program_uniform1fv_is_derived(self) -> None:
        # glProgramUniform1fv's own shape: (program, location, count, value).
        param = Parameter(name='value', type='const GLfloat *', len='count')
        other = [_program_param(), _location_param(), _count_param()]

        override = derive_uniform_setter_override('glProgramUniform1fv', param, other)

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.direction, PointerDirection.IN)
        self.assertEqual(override.length_multiplier, 1)

    def test_program_uniform4uiv_is_derived(self) -> None:
        param = Parameter(name='value', type='const GLuint *', len='count*4')
        other = [_program_param(), _location_param(), _count_param()]

        override = derive_uniform_setter_override('glProgramUniform4uiv', param, other)

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.length_multiplier, 4)

    def test_wrong_param_name_is_not_derived(self) -> None:
        param = Parameter(name='pointer', type='const GLfloat *', len='count*4')
        other = [_location_param(), _count_param()]

        self.assertIsNone(derive_uniform_setter_override('glUniform4fv', param, other))

    def test_vendor_suffixed_name_is_not_derived(self) -> None:
        # glUniform1fARB is a scalar setter, not a vector one, and even a hypothetical
        # glUniform1fvARB must not match - the regex is fully anchored (`v$`, nothing after).
        param = Parameter(name='value', type='const GLfloat *', len='count')
        other = [_location_param(), _count_param()]

        self.assertIsNone(derive_uniform_setter_override('glUniform1fvARB', param, other))

    def test_64bit_integer_family_is_not_derived(self) -> None:
        # glUniform1i64vARB - a different, newer value-type domain (GLint64) this module
        # deliberately never extends to (see this module's own header comment).
        param = Parameter(name='value', type='const GLint64 *', len='count')
        other = [_location_param(), _count_param()]

        self.assertIsNone(derive_uniform_setter_override('glUniform1i64vARB', param, other))

    def test_bindless_handle_family_is_not_derived(self) -> None:
        param = Parameter(name='value', type='const GLuint64 *', len='count')
        other = [_location_param(), _count_param()]

        self.assertIsNone(
            derive_uniform_setter_override('glProgramUniformHandleui64vARB', param, other))

    def test_missing_location_sibling_is_not_derived(self) -> None:
        param = Parameter(name='value', type='const GLfloat *', len='count*4')
        other = [_count_param()]  # no 'location'

        self.assertIsNone(derive_uniform_setter_override('glUniform4fv', param, other))

    def test_missing_count_sibling_is_not_derived(self) -> None:
        param = Parameter(name='value', type='const GLfloat *', len='count*4')
        other = [_location_param()]  # no 'count'

        self.assertIsNone(derive_uniform_setter_override('glUniform4fv', param, other))

    def test_non_scalar_count_sibling_is_not_derived(self) -> None:
        param = Parameter(name='value', type='const GLfloat *', len='count*4')
        other = [_location_param(), Parameter(name='count', type='GLsizei *')]  # not scalar

        self.assertIsNone(derive_uniform_setter_override('glUniform4fv', param, other))

    def test_len_disagreeing_with_name_is_not_derived(self) -> None:
        # A hypothetical future gl.xml revision changing glUniform4fv's len= to 'count*3'
        # without renaming the command - the cross-check between the name-implied multiplier
        # (4) and gl.xml's own len=-derived multiplier (3) must reject this, not silently pick
        # one side.
        param = Parameter(name='value', type='const GLfloat *', len='count*3')
        other = [_location_param(), _count_param()]

        self.assertIsNone(derive_uniform_setter_override('glUniform4fv', param, other))

    def test_compsize_len_is_not_derived(self) -> None:
        # Not a real member of this family at all - a param happening to be named 'value'
        # with a COMPSIZE(...) len (e.g. glClearBufferfv's own `value`) must never be derived
        # just because the param name matches; the command name itself must also match one of
        # this module's two family regexes (glClearBufferfv does not).
        param = Parameter(name='value', type='const GLfloat *', len='COMPSIZE(buffer)')
        other = [Parameter(name='buffer', type='GLenum')]

        self.assertIsNone(derive_uniform_setter_override('glClearBufferfv', param, other))

    def test_unrelated_command_name_is_not_derived(self) -> None:
        param = Parameter(name='value', type='const GLfloat *', len='count*4')
        other = [_location_param(), _count_param()]

        self.assertIsNone(derive_uniform_setter_override('glSomeUnrelatedCommand', param, other))


class DeriveUniformSetterOverrideMatrixUnitTests(unittest.TestCase):
    """Direct unit tests against derive_uniform_setter_override() for the matrix
    (glUniformMatrix*v/glProgramUniformMatrix*v) family."""

    def test_uniform_matrix2dv_is_derived_with_multiplier_four(self) -> None:
        param = Parameter(name='value', type='const GLdouble *', len='count*4')
        other = [_location_param(), _count_param(), _transpose_param()]

        override = derive_uniform_setter_override('glUniformMatrix2dv', param, other)

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.direction, PointerDirection.IN)
        self.assertEqual(override.length_source_kind, LengthSourceKind.SIBLING_PARAM)
        self.assertEqual(override.length_source_value, 'count')
        self.assertEqual(override.length_multiplier, 4)

    def test_uniform_matrix3dv_is_derived_with_multiplier_nine(self) -> None:
        param = Parameter(name='value', type='const GLdouble *', len='count*9')
        other = [_location_param(), _count_param(), _transpose_param()]

        override = derive_uniform_setter_override('glUniformMatrix3dv', param, other)

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.length_multiplier, 9)

    def test_non_square_matrix2x3fv_is_derived_with_multiplier_six(self) -> None:
        param = Parameter(name='value', type='const GLfloat *', len='count*6')
        other = [_location_param(), _count_param(), _transpose_param()]

        override = derive_uniform_setter_override('glUniformMatrix2x3fv', param, other)

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.length_multiplier, 6)

    def test_non_square_matrix4x3dv_is_derived_with_multiplier_twelve(self) -> None:
        param = Parameter(name='value', type='const GLdouble *', len='count*12')
        other = [_location_param(), _count_param(), _transpose_param()]

        override = derive_uniform_setter_override('glUniformMatrix4x3dv', param, other)

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.length_multiplier, 12)

    def test_program_uniform_matrix2fv_is_derived(self) -> None:
        param = Parameter(name='value', type='const GLfloat *', len='count*4')
        other = [_program_param(), _location_param(), _count_param(), _transpose_param()]

        override = derive_uniform_setter_override('glProgramUniformMatrix2fv', param, other)

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.length_multiplier, 4)

    def test_program_uniform_matrix3x2dv_is_derived(self) -> None:
        param = Parameter(name='value', type='const GLdouble *', len='count*6')
        other = [_program_param(), _location_param(), _count_param(), _transpose_param()]

        override = derive_uniform_setter_override('glProgramUniformMatrix3x2dv', param, other)

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.length_multiplier, 6)

    def test_missing_transpose_sibling_is_not_derived(self) -> None:
        # Structurally missing the one sibling that distinguishes the matrix family from the
        # vector family - must not be derived even though the name matches.
        param = Parameter(name='value', type='const GLfloat *', len='count*4')
        other = [_location_param(), _count_param()]  # no 'transpose'

        self.assertIsNone(derive_uniform_setter_override('glUniformMatrix2fv', param, other))

    def test_non_square_vendor_suffixed_name_is_not_derived(self) -> None:
        # glUniformMatrix2x3fvNV - a vendor-suffixed alias, excluded exactly like the vector
        # family's own ARB suffix exclusion.
        param = Parameter(name='value', type='const GLfloat *', len='count*6')
        other = [_location_param(), _count_param(), _transpose_param()]

        self.assertIsNone(
            derive_uniform_setter_override('glUniformMatrix2x3fvNV', param, other))

    def test_matrix_len_disagreeing_with_name_is_not_derived(self) -> None:
        param = Parameter(name='value', type='const GLfloat *', len='count*9')  # should be 4
        other = [_location_param(), _count_param(), _transpose_param()]

        self.assertIsNone(derive_uniform_setter_override('glUniformMatrix2fv', param, other))


class PointerOverrideForFallbackTests(unittest.TestCase):
    """generator_helpers.py's pointer_override_for() must consult this fallback (after the
    pixel-transfer one), and an explicit POINTER_PARAM_OVERRIDES entry must still take priority
    over it."""

    def _make_command(self, name: str, params: list[Parameter]) -> Command:
        return Command(
            name=name, api=Api.GL, enabled=True, command_type=CmdType.PARAM,
            return_value=ReturnValue(type='void'), params=params)

    def test_fallback_is_used_when_no_explicit_entry_exists(self) -> None:
        value = Parameter(name='value', type='const GLdouble *', len='count*2')
        cmd = self._make_command('glUniform2dv', [_location_param(), _count_param(), value])
        self.assertNotIn((cmd.name, 'value'), POINTER_PARAM_OVERRIDES)

        override = pointer_override_for(cmd, 'value')

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.length_source_kind, LengthSourceKind.SIBLING_PARAM)
        self.assertEqual(override.length_multiplier, 2)

    def test_unknown_param_name_returns_none(self) -> None:
        cmd = self._make_command('glUniform2dv', [_location_param()])
        self.assertIsNone(pointer_override_for(cmd, 'does_not_exist'))


class EndToEndUniformSetterTests(unittest.TestCase):
    """Confirms real, live gl.xml commands actually get auto-derived (or correctly do NOT),
    mirroring test_pixel_transfer_metadata.py's EndToEndPixelTransferTests pattern."""

    def _get_command(self, api: Api, name: str) -> Command:
        registry = load_registry(api)
        raw_commands = extract_commands_from_registry(registry)
        raw = next(c for c in raw_commands if c['command_attribs']['name'] == name)
        return get_commands_from_xml_data({api: [raw]})[api][0]

    def test_previously_manual_commands_have_no_explicit_table_entry(self) -> None:
        # See pointer_metadata.py's own comment: these manual entries were removed once this
        # fallback covered them - pinning that removal down as a regression check.
        for name in (
            'glUniform1fv', 'glUniform2fv', 'glUniform3fv', 'glUniform4fv',
            'glUniform1iv', 'glUniform2iv', 'glUniform3iv', 'glUniform4iv',
            'glUniform1uiv', 'glUniform2uiv', 'glUniform3uiv', 'glUniform4uiv',
            'glUniformMatrix2fv', 'glUniformMatrix3fv', 'glUniformMatrix4fv',
            'glUniform4dv', 'glUniformMatrix4dv',
            'glProgramUniform4dv', 'glProgramUniformMatrix4dv', 'glProgramUniformMatrix4fv',
        ):
            with self.subTest(command=name):
                self.assertNotIn((name, 'value'), POINTER_PARAM_OVERRIDES)

    def test_previously_manual_commands_are_still_enabled(self) -> None:
        for name in (
            'glUniform1fv', 'glUniform2fv', 'glUniform3fv', 'glUniform4fv',
            'glUniform1iv', 'glUniform2iv', 'glUniform3iv', 'glUniform4iv',
            'glUniform1uiv', 'glUniform2uiv', 'glUniform3uiv', 'glUniform4uiv',
            'glUniformMatrix2fv', 'glUniformMatrix3fv', 'glUniformMatrix4fv',
            'glUniform4dv', 'glUniformMatrix4dv',
            'glProgramUniform4dv', 'glProgramUniformMatrix4dv', 'glProgramUniformMatrix4fv',
        ):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_newly_covered_double_vector_setters_are_enabled(self) -> None:
        # glUniform1dv/2dv/3dv: same family as the already-classified glUniform4dv, just the
        # missing narrower vector widths - never had a hand entry before this change.
        for name in ('glUniform1dv', 'glUniform2dv', 'glUniform3dv'):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_newly_covered_double_matrix_setters_are_enabled(self) -> None:
        for name in ('glUniformMatrix2dv', 'glUniformMatrix3dv'):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_newly_covered_non_square_matrix_setters_are_enabled(self) -> None:
        for name in (
            'glUniformMatrix2x3fv', 'glUniformMatrix2x4fv', 'glUniformMatrix3x2fv',
            'glUniformMatrix3x4fv', 'glUniformMatrix4x2fv', 'glUniformMatrix4x3fv',
            'glUniformMatrix2x3dv', 'glUniformMatrix2x4dv', 'glUniformMatrix3x2dv',
            'glUniformMatrix3x4dv', 'glUniformMatrix4x2dv', 'glUniformMatrix4x3dv',
        ):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_newly_covered_dsa_vector_setters_are_enabled(self) -> None:
        # glProgramUniform{1,2,3,4}{f,i,ui}v: the DSA counterparts of the already-classified
        # glUniform*v family - only the "4"-suffixed *matrix*/double variants had hand entries
        # before this change (glProgramUniform4dv/glProgramUniformMatrix4{d,f}v); the plain
        # float/int/uint DSA vector setters never had any entry at all.
        for n in (1, 2, 3, 4):
            for suffix in ('f', 'i', 'ui'):
                name = f'glProgramUniform{n}{suffix}v'
                with self.subTest(command=name):
                    self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_newly_covered_dsa_double_vector_setters_are_enabled(self) -> None:
        for n in (1, 2, 3):
            name = f'glProgramUniform{n}dv'
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_newly_covered_dsa_matrix_setters_are_enabled(self) -> None:
        for name in (
            'glProgramUniformMatrix2fv', 'glProgramUniformMatrix3fv',
            'glProgramUniformMatrix2dv', 'glProgramUniformMatrix3dv',
            'glProgramUniformMatrix2x3fv', 'glProgramUniformMatrix2x4fv',
            'glProgramUniformMatrix3x2fv', 'glProgramUniformMatrix3x4fv',
            'glProgramUniformMatrix4x2fv', 'glProgramUniformMatrix4x3fv',
            'glProgramUniformMatrix2x3dv', 'glProgramUniformMatrix2x4dv',
            'glProgramUniformMatrix3x2dv', 'glProgramUniformMatrix3x4dv',
            'glProgramUniformMatrix4x2dv', 'glProgramUniformMatrix4x3dv',
        ):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_vendor_suffixed_siblings_stay_disabled(self) -> None:
        # ARB/EXT/NV aliases of already-enabled commands must not be swept in just because
        # they share a prefix - the family regex is fully anchored.
        for name in (
            'glUniform1fvARB', 'glUniform2fvARB', 'glUniform3fvARB', 'glUniform4fvARB',
            'glUniformMatrix2fvARB', 'glUniformMatrix3fvARB', 'glUniformMatrix4fvARB',
            'glUniformMatrix2x3fvNV', 'glUniformMatrix2x4fvNV', 'glUniformMatrix3x2fvNV',
            'glUniformMatrix3x4fvNV', 'glUniformMatrix4x2fvNV', 'glUniformMatrix4x3fvNV',
            'glProgramUniform1fvEXT', 'glProgramUniformMatrix2fvEXT',
        ):
            with self.subTest(command=name):
                self.assertFalse(self._get_command(Api.GL, name).enabled)

    def test_64bit_and_bindless_handle_vector_setters_stay_disabled(self) -> None:
        # The *vector* (array-parameter) forms of the 64-bit-integer/bindless-handle families -
        # a genuinely different value-type domain this module deliberately never extends to
        # (see uniform_setter_metadata.py's header comment). Note this is specifically about
        # the 'v'-suffixed array forms: their plain scalar siblings (glProgramUniform1i64ARB,
        # glUniformHandleui64ARB, ...) have no pointer parameter at all and are already
        # enabled=True on their own, unrelated to this module.
        for name in (
            'glUniform1i64vARB', 'glUniform1ui64vARB',
            'glProgramUniformHandleui64vARB',
        ):
            with self.subTest(command=name):
                self.assertFalse(self._get_command(Api.GL, name).enabled)


if __name__ == '__main__':
    unittest.main()
