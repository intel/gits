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

from generator_helpers import pointer_override_for
from intermediates import Api, Command, CmdType, Parameter, ReturnValue
from intermediates_creator import (
    FILE_TO_API,
    _is_param_capturable,  # pyright: ignore[reportPrivateUsage]
    extract_commands_from_registry,
    get_commands_from_xml_data,
)
from pointer_metadata import (LengthSourceKind, PointerDirection, POINTER_PARAM_OVERRIDES,
                              TexOrSamplerParameterFamily)
from registry import Registry
from tex_sampler_parameter_metadata import derive_tex_sampler_parameter_override

REGISTRY_PATHS: dict[Api, Path] = {
    api: path for path, api in FILE_TO_API.items()
}


def load_registry(api: Api) -> Registry:
    registry = Registry()
    registry.loadFile(REGISTRY_PATHS[api])
    return registry


def _pname_param() -> Parameter:
    return Parameter(name='pname', type='GLenum')


def _target_param() -> Parameter:
    return Parameter(name='target', type='GLenum')


def _sampler_param() -> Parameter:
    return Parameter(name='sampler', type='GLuint')


class DeriveTexSamplerParameterOverrideUnitTests(unittest.TestCase):
    """Direct unit tests against derive_tex_sampler_parameter_override() - no registry I/O, so
    every accept/reject boundary from that function's own header comment is pinned down
    precisely."""

    def test_tex_parameter_iiv_is_derived(self) -> None:
        param = Parameter(name='params', type='const GLint *', len='COMPSIZE(pname)')
        other = [_target_param(), _pname_param()]

        override = derive_tex_sampler_parameter_override('glTexParameterIiv', param, other)

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.direction, PointerDirection.IN)
        self.assertEqual(override.length_source_kind, LengthSourceKind.TEX_SAMPLER_PARAMETER_VALUE_COUNT)
        self.assertEqual(override.pname_param, 'pname')

    def test_tex_parameter_iuiv_is_derived(self) -> None:
        param = Parameter(name='params', type='const GLuint *', len='COMPSIZE(pname)')
        other = [_target_param(), _pname_param()]

        override = derive_tex_sampler_parameter_override('glTexParameterIuiv', param, other)

        self.assertIsNotNone(override)

    def test_sampler_parameteriv_is_derived(self) -> None:
        param = Parameter(name='param', type='const GLint *', len='COMPSIZE(pname)')
        other = [_sampler_param(), _pname_param()]

        override = derive_tex_sampler_parameter_override('glSamplerParameteriv', param, other)

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.direction, PointerDirection.IN)
        self.assertEqual(override.pname_param, 'pname')

    def test_sampler_parameter_iiv_is_derived(self) -> None:
        param = Parameter(name='param', type='const GLint *', len='COMPSIZE(pname)')
        other = [_sampler_param(), _pname_param()]

        override = derive_tex_sampler_parameter_override('glSamplerParameterIiv', param, other)

        self.assertIsNotNone(override)

    def test_sampler_parameter_iuiv_is_derived(self) -> None:
        param = Parameter(name='param', type='const GLuint *', len='COMPSIZE(pname)')
        other = [_sampler_param(), _pname_param()]

        override = derive_tex_sampler_parameter_override('glSamplerParameterIuiv', param, other)

        self.assertIsNotNone(override)

    def test_previously_manual_tex_parameterfv_is_still_derived(self) -> None:
        # glTexParameterfv/iv/glSamplerParameterfv used to have manual table entries - this
        # module must derive the identical override now that they are gone.
        param = Parameter(name='params', type='const GLfloat *', len='COMPSIZE(pname)')
        other = [_target_param(), _pname_param()]

        override = derive_tex_sampler_parameter_override('glTexParameterfv', param, other)

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.direction, PointerDirection.IN)
        self.assertEqual(override.length_source_kind, LengthSourceKind.TEX_SAMPLER_PARAMETER_VALUE_COUNT)

    def test_wrong_param_name_is_not_derived(self) -> None:
        # 'param' (singular) is the Sampler- convention, not the Tex- one.
        param = Parameter(name='param', type='const GLint *', len='COMPSIZE(pname)')
        other = [_target_param(), _pname_param()]

        self.assertIsNone(derive_tex_sampler_parameter_override('glTexParameteriv', param, other))

    def test_get_variant_is_not_derived(self) -> None:
        # glGetTexParameteriv shares the identical COMPSIZE(pname) shape, but no OUT-direction
        # Mako branch exists for TEX_SAMPLER_PARAMETER_VALUE_COUNT - see this module's own
        # header comment for exactly why this is deliberately excluded, not merely unaudited.
        param = Parameter(name='params', type='GLint *', len='COMPSIZE(pname)')
        other = [_target_param(), _pname_param()]

        self.assertIsNone(derive_tex_sampler_parameter_override('glGetTexParameteriv', param, other))

    def test_fixed_point_x_suffix_is_not_derived(self) -> None:
        # glTexParameterxv (GLES/OES fixed-point) - the type-suffix group is (f|i|ui), never x.
        param = Parameter(name='params', type='const GLfixed *', len='COMPSIZE(pname)')
        other = [_target_param(), _pname_param()]

        self.assertIsNone(derive_tex_sampler_parameter_override('glTexParameterxv', param, other))

    def test_texture_dsa_variant_is_tex_parameter_family(self) -> None:
        param = Parameter(name='param', type='const GLfloat *', len='COMPSIZE(pname)')
        other = [Parameter(name='texture', type='GLuint'), _pname_param()]

        override = derive_tex_sampler_parameter_override('glTextureParameterfv', param, other)

        assert override is not None
        self.assertEqual(override.tex_sampler_family, TexOrSamplerParameterFamily.TEX_PARAMETER)

    def test_texture_dsa_get_variant_is_not_derived(self) -> None:
        param = Parameter(name='params', type='GLfloat *', len='COMPSIZE(pname)')
        other = [Parameter(name='texture', type='GLuint'), _pname_param()]

        self.assertIsNone(
            derive_tex_sampler_parameter_override('glGetTextureParameterfv', param, other))

    def test_multi_tex_ext_variant_is_not_derived(self) -> None:
        param = Parameter(name='params', type='const GLfloat *', len='COMPSIZE(pname)')
        other = [Parameter(name='texunit', type='GLenum'), _target_param(), _pname_param()]

        self.assertIsNone(
            derive_tex_sampler_parameter_override('glMultiTexParameterfvEXT', param, other))

    def test_vendor_suffixed_name_is_not_derived(self) -> None:
        param = Parameter(name='params', type='const GLint *', len='COMPSIZE(pname)')
        other = [_target_param(), _pname_param()]

        self.assertIsNone(
            derive_tex_sampler_parameter_override('glTexParameterIivEXT', param, other))

    def test_missing_pname_sibling_is_not_derived(self) -> None:
        param = Parameter(name='params', type='const GLint *', len='COMPSIZE(pname)')
        other = [_target_param()]  # no 'pname'

        self.assertIsNone(
            derive_tex_sampler_parameter_override('glTexParameterIiv', param, other))

    def test_non_scalar_pname_sibling_is_not_derived(self) -> None:
        param = Parameter(name='params', type='const GLint *', len='COMPSIZE(pname)')
        other = [_target_param(), Parameter(name='pname', type='GLenum *')]  # not scalar

        self.assertIsNone(
            derive_tex_sampler_parameter_override('glTexParameterIiv', param, other))

    def test_non_compsize_pname_len_is_not_derived(self) -> None:
        # A hypothetical future gl.xml revision changing this len= away from the exact
        # "COMPSIZE(pname)" shape every audited sibling carries.
        param = Parameter(name='params', type='const GLint *', len='COMPSIZE(pname,target)')
        other = [_target_param(), _pname_param()]

        self.assertIsNone(
            derive_tex_sampler_parameter_override('glTexParameterIiv', param, other))

    def test_unrelated_command_name_is_not_derived(self) -> None:
        param = Parameter(name='params', type='const GLint *', len='COMPSIZE(pname)')
        other = [_target_param(), _pname_param()]

        self.assertIsNone(
            derive_tex_sampler_parameter_override('glSomeUnrelatedCommand', param, other))


class PointerOverrideForFallbackTests(unittest.TestCase):
    """generator_helpers.py's pointer_override_for() must consult this fallback (after the
    pixel-transfer and uniform-setter ones), and an explicit POINTER_PARAM_OVERRIDES entry must
    still take priority over it."""

    def _make_command(self, name: str, params: list[Parameter]) -> Command:
        return Command(
            name=name, api=Api.GL, enabled=True, command_type=CmdType.PARAM,
            return_value=ReturnValue(type='void'), params=params)

    def test_fallback_is_used_when_no_explicit_entry_exists(self) -> None:
        params = Parameter(name='params', type='const GLint *', len='COMPSIZE(pname)')
        cmd = self._make_command('glTexParameterIiv', [_target_param(), _pname_param(), params])
        self.assertNotIn((cmd.name, 'params'), POINTER_PARAM_OVERRIDES)

        override = pointer_override_for(cmd, 'params')

        self.assertIsNotNone(override)
        assert override is not None
        self.assertEqual(override.length_source_kind, LengthSourceKind.TEX_SAMPLER_PARAMETER_VALUE_COUNT)
        self.assertEqual(override.pname_param, 'pname')

    def test_unknown_param_name_returns_none(self) -> None:
        cmd = self._make_command('glTexParameterIiv', [_target_param()])
        self.assertIsNone(pointer_override_for(cmd, 'does_not_exist'))


class EndToEndTexSamplerParameterTests(unittest.TestCase):
    """Confirms real, live gl.xml commands actually get auto-derived (or correctly do NOT),
    mirroring test_uniform_setter_metadata.py's EndToEndUniformSetterTests pattern."""

    def _get_command(self, api: Api, name: str) -> Command:
        registry = load_registry(api)
        raw_commands = extract_commands_from_registry(registry)
        raw = next(c for c in raw_commands if c['command_attribs']['name'] == name)
        return get_commands_from_xml_data({api: [raw]})[api][0]

    def test_previously_manual_commands_have_no_explicit_table_entry(self) -> None:
        for name, param_name in (
            ('glTexParameterfv', 'params'),
            ('glTexParameteriv', 'params'),
            ('glSamplerParameterfv', 'param'),
        ):
            with self.subTest(command=name):
                self.assertNotIn((name, param_name), POINTER_PARAM_OVERRIDES)

    def test_previously_manual_commands_are_still_enabled(self) -> None:
        for name in ('glTexParameterfv', 'glTexParameteriv', 'glSamplerParameterfv'):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_newly_covered_integer_variants_are_enabled(self) -> None:
        for name in (
            'glTexParameterIiv', 'glTexParameterIuiv',
            'glSamplerParameteriv', 'glSamplerParameterIiv', 'glSamplerParameterIuiv',
        ):
            with self.subTest(command=name):
                self.assertTrue(self._get_command(Api.GL, name).enabled)

    def test_get_variants_use_discarded_output(self) -> None:
        # Same COMPSIZE(pname) shape, but no OUT-direction Mako branch exists for this
        # LengthSourceKind - see tex_sampler_parameter_metadata.py's own header comment. The
        # getters are enabled through the generic DISCARDED_OUTPUT fallback instead.
        from pointer_metadata import LengthSourceKind
        for name in (
            'glGetTexParameterfv', 'glGetTexParameteriv',
            'glGetTexParameterIiv', 'glGetTexParameterIuiv',
            'glGetSamplerParameterfv', 'glGetSamplerParameteriv',
            'glGetSamplerParameterIiv', 'glGetSamplerParameterIuiv',
        ):
            with self.subTest(command=name):
                cmd = self._get_command(Api.GL, name)
                self.assertTrue(cmd.enabled)
                override = pointer_override_for(cmd, 'params')
                self.assertEqual(override.length_source_kind, LengthSourceKind.DISCARDED_OUTPUT)

    def test_dsa_variants_are_tex_parameter_family(self) -> None:
        for name, param_name in (
            ('glTextureParameterfv', 'param'), ('glTextureParameteriv', 'param'),
            ('glTextureParameterIiv', 'params'), ('glTextureParameterIuiv', 'params'),
            ('glTextureParameterfvEXT', 'params'), ('glTextureParameterivEXT', 'params'),
            ('glTextureParameterIivEXT', 'params'), ('glTextureParameterIuivEXT', 'params'),
        ):
            with self.subTest(command=name):
                cmd = self._get_command(Api.GL, name)
                self.assertTrue(cmd.enabled)
                override = pointer_override_for(cmd, param_name)
                self.assertEqual(override.length_source_kind,
                                 LengthSourceKind.TEX_SAMPLER_PARAMETER_VALUE_COUNT)
                self.assertEqual(override.tex_sampler_family,
                                 TexOrSamplerParameterFamily.TEX_PARAMETER)

    def test_fixed_point_and_multitex_variants_stay_disabled(self) -> None:
        for name in (
            'glTexParameterxv', 'glTexParameterxvOES',
            'glMultiTexParameterfvEXT', 'glMultiTexParameterivEXT',
            'glTexParameterIivEXT', 'glTexParameterIivOES',
        ):
            with self.subTest(command=name):
                self.assertFalse(self._get_command(Api.GL, name).enabled)


# gits::opengl2::ComputeTexOrSamplerParameterValueCount() (the C++ helper that actually answers
# "how many elements does this pname occupy", consulted at capture time by every command this
# module derives an override for) is hand-written, not generated - there is no Mako template and
# no other Python module driving its pname-to-count switch. Unlike everything above (which tests
# derive_tex_sampler_parameter_override()'s command-name-driven family derivation), this class
# instead pins down two GL_TEXTURE_CROP_RECT_OES-specific facts directly against source text and
# the real registry, mirroring this module's own EndToEndTexSamplerParameterTests pattern of
# checking real, live gl.xml data rather than a hand-copied assumption:
#   1. The registry fact this codebase's own case comment for GL_TEXTURE_CROP_RECT_OES relies
#      on (it carries no `group=` attribute at all, unlike GL_TEXTURE_SWIZZLE_RGBA's
#      group="TextureParameterName") - if a future gl.xml revision changes this, this test flags
#      it so the C++ comment's own reasoning can be revisited.
#   2. The hand-written switch in pixelTransferSize.cpp actually has a case for this pname - a
#      lightweight regression guard against that case silently disappearing (e.g. an
#      unrelated future edit to the same switch), since no C++ test executable exists in this
#      backend to exercise the runtime helper itself (see AGENTS.md's build-only verification
#      convention for this backend during active development).
class TexOrSamplerParameterValueCountCppHelperTests(unittest.TestCase):
    """Source-text/registry regression coverage for GL_TEXTURE_CROP_RECT_OES's 4-element case in
    the hand-written gits::opengl2::ComputeTexOrSamplerParameterValueCount()."""

    _PIXEL_TRANSFER_SIZE_CPP = (
        CODEGEN_DIR.parent / 'common' / 'layer_interface' / 'pixelTransferSize.cpp')

    def test_registry_confirms_crop_rect_oes_has_no_texture_parameter_name_group(self) -> None:
        registry = load_registry(Api.GL)
        enum = registry.enumdict.get('GL_TEXTURE_CROP_RECT_OES')
        self.assertIsNotNone(enum, 'GL_TEXTURE_CROP_RECT_OES missing from the vendored gl.xml')
        assert enum is not None
        self.assertEqual(enum.get('value'), '0x8B9D')
        # Unlike GL_TEXTURE_SWIZZLE_RGBA (group="TextureParameterName"), this enum is untagged -
        # ComputeTexOrSamplerParameterValueCount()'s own case comment for it rests on the
        # legacy (non-OpenGL2) backend's precedent instead, not a gl.xml group tag. Pinned here
        # so that fact stays true, or this test (and that comment) get revisited together.
        self.assertIsNone(enum.get('group'))

    def test_cpp_helper_has_a_four_element_case_for_crop_rect_oes(self) -> None:
        source = self._PIXEL_TRANSFER_SIZE_CPP.read_text(encoding='utf-8')
        # Matches "case 0x8B9D:" (whitespace/case-insensitive on the hex digits) followed,
        # somewhere before the next "case"/"default", by a kTexParameter ?: 4 : ... ternary -
        # the exact shape GL_TEXTURE_SWIZZLE_RGBA's own case already uses just above it.
        case_match = re.search(r'case 0x8[Bb]9[Dd]:(.*?)(?=\n  case |\n  default:)', source,
                               re.DOTALL)
        self.assertIsNotNone(
            case_match, 'No "case 0x8B9D" (GL_TEXTURE_CROP_RECT_OES) found in '
            'ComputeTexOrSamplerParameterValueCount()')
        assert case_match is not None
        case_body = case_match.group(1)
        self.assertIn('kTexParameter', case_body)
        self.assertIn('4', case_body)


if __name__ == '__main__':
    unittest.main()
