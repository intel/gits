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

from intermediates import Api, Parameter
from intermediates_creator import (
    FILE_TO_API,
    extract_commands_from_registry,
    get_commands_from_xml_data,
)
from pointer_metadata import LengthSourceKind
from pointer_length_derivation import (
    LenAttribShape,
    LengthSourceDerivation,
    classify_len_attrib,
    derive_length_source,
    derive_pointer_param_override,
    find_pointer_length_mismatches,
)
from registry import Registry

REGISTRY_PATHS: dict[Api, Path] = {
    api: path for path, api in FILE_TO_API.items()
}


def load_registry(api: Api) -> Registry:
    registry = Registry()
    registry.loadFile(REGISTRY_PATHS[api])
    return registry


def load_all_commands() -> dict[Api, list]:
    """Load every command from every registry - mirrors generator.py's
    load_registries_and_extract_xml_data(), but only what find_pointer_length_mismatches()
    needs, so this test doesn't have to also load/process enums."""
    raw_by_api = {}
    for path, api in FILE_TO_API.items():
        registry = load_registry(api)
        raw_by_api[api] = extract_commands_from_registry(registry)
    return get_commands_from_xml_data(raw_by_api)


class ClassifyLenAttribTests(unittest.TestCase):
    def test_none_is_none_shape(self) -> None:
        self.assertEqual(classify_len_attrib(None), LenAttribShape.NONE)

    def test_integer_literal(self) -> None:
        self.assertEqual(classify_len_attrib('4'), LenAttribShape.INTEGER)

    def test_plain_sibling_name(self) -> None:
        self.assertEqual(classify_len_attrib('size'), LenAttribShape.PARAM_NAME)
        self.assertEqual(classify_len_attrib('count'), LenAttribShape.PARAM_NAME)

    def test_scaled_sibling_name(self) -> None:
        self.assertEqual(classify_len_attrib('count*4'), LenAttribShape.SCALED_PARAM_NAME)
        self.assertEqual(classify_len_attrib('count * 4'), LenAttribShape.SCALED_PARAM_NAME)

    def test_divided_sibling_name(self) -> None:
        self.assertEqual(classify_len_attrib('bufSize / 4'), LenAttribShape.DIVIDED_PARAM_NAME)
        self.assertEqual(classify_len_attrib('dataSize/4'), LenAttribShape.DIVIDED_PARAM_NAME)

    def test_compsize(self) -> None:
        self.assertEqual(classify_len_attrib('COMPSIZE(pname)'), LenAttribShape.COMPSIZE)
        self.assertEqual(
            classify_len_attrib('COMPSIZE(format,type,width,height)'), LenAttribShape.COMPSIZE)
        self.assertEqual(classify_len_attrib('COMPSIZE()'), LenAttribShape.COMPSIZE)

    def test_unrecognized_shape_raises(self) -> None:
        with self.assertRaises(ValueError):
            classify_len_attrib('something gl.xml has never actually used')


class DeriveLengthSourceTests(unittest.TestCase):
    def test_integer_derives_fixed_count(self) -> None:
        param = Parameter(name='ppfd', type='const PIXELFORMATDESCRIPTOR *', len='1')
        derivation = derive_length_source(param, other_params=[])
        self.assertEqual(
            derivation, LengthSourceDerivation(kind=LengthSourceKind.FIXED_COUNT, value='1'))

    def test_param_name_with_valid_scalar_sibling_derives_sibling_param(self) -> None:
        param = Parameter(name='data', type='const void *', len='size')
        sibling = Parameter(name='size', type='GLsizeiptr')
        derivation = derive_length_source(param, other_params=[sibling])
        self.assertEqual(
            derivation, LengthSourceDerivation(kind=LengthSourceKind.SIBLING_PARAM, value='size'))

    def test_param_name_with_missing_sibling_derives_nothing(self) -> None:
        param = Parameter(name='data', type='const void *', len='size')
        self.assertIsNone(derive_length_source(param, other_params=[]))

    def test_param_name_with_non_scalar_sibling_derives_nothing(self) -> None:
        # Mirrors the one real registry case this guards against: glExtGetProgramBinarySourceQCOM's
        # `source` names `length` as its len= sibling, but `length` is itself an OUT pointer,
        # not a by-value scalar - a fundamentally different shape from an ordinary sibling-count
        # reference.
        param = Parameter(name='source', type='GLchar *', len='length')
        sibling = Parameter(name='length', type='GLint *')  # not scalar - itself a pointer
        self.assertIsNone(derive_length_source(param, other_params=[sibling]))

    def test_scaled_param_name_derives_sibling_param_with_multiplier(self) -> None:
        param = Parameter(name='value', type='const GLfloat *', len='count*4')
        sibling = Parameter(name='count', type='GLsizei')
        derivation = derive_length_source(param, other_params=[sibling])
        self.assertEqual(
            derivation,
            LengthSourceDerivation(
                kind=LengthSourceKind.SIBLING_PARAM, value='count', multiplier=4))

    def test_scaled_param_name_with_invalid_sibling_derives_nothing(self) -> None:
        param = Parameter(name='value', type='const GLfloat *', len='count*4')
        sibling = Parameter(name='count', type='GLsizei *')  # not scalar
        self.assertIsNone(derive_length_source(param, other_params=[sibling]))

    def test_divided_param_name_derives_nothing(self) -> None:
        # See classify_len_attrib()'s own comment: bufSize/4-style forms are a caller-supplied
        # OUT buffer CAPACITY (a safety upper bound), never the actual written-element count -
        # unlike count*4's caller-supplied exact element count.
        param = Parameter(name='params', type='GLfloat *', len='bufSize / 4')
        sibling = Parameter(name='bufSize', type='GLsizei')
        self.assertIsNone(derive_length_source(param, other_params=[sibling]))

    def test_compsize_derives_nothing(self) -> None:
        # glGetShaderiv-style: gl.xml's own COMPSIZE(pname) hint is actively misleading here
        # (implies a variable count; the real answer is always exactly 1) - must never be
        # trusted as a length by itself.
        param = Parameter(name='params', type='GLint *', len='COMPSIZE(pname)')
        pname = Parameter(name='pname', type='GLenum')
        self.assertIsNone(derive_length_source(param, other_params=[pname]))

    def test_no_len_attrib_derives_nothing(self) -> None:
        param = Parameter(name='name', type='const GLchar *')
        self.assertIsNone(derive_length_source(param, other_params=[]))


class DerivePointerParamOverrideTests(unittest.TestCase):
    def test_always_returns_none_today(self) -> None:
        # No safe category (direction/dereference-semantics) carve-out exists yet - see this
        # function's own docstring for exactly why and what would need to change. Exercised
        # against a real, currently-unclassified command/param shape (glProgramUniform4dv's
        # `value`, len="count*4" - structurally identical to glUniform4fv's own hand-classified
        # `value`) to confirm this stays a documented "not yet", not a silent guess, even for a
        # case that looks safe by analogy.
        param = Parameter(name='value', type='const GLdouble *', len='count*4')
        sibling = Parameter(name='count', type='GLsizei')
        self.assertIsNone(
            derive_pointer_param_override('glProgramUniform4dv', param, [sibling]))


class FindPointerLengthMismatchesTests(unittest.TestCase):
    """End-to-end: cross-validates every real POINTER_PARAM_OVERRIDES/
    ARRAY_OF_ARRAYS_PARAM_OVERRIDES entry against real registry data - mirrors
    test_pointer_metadata.py's EndToEndEnabledGateTests pattern."""

    def test_no_mismatches_against_current_hand_tables(self) -> None:
        # Pins down the investigation's own finding: every existing hand-authored entry's
        # length-source already agrees with what gl.xml's len= independently derives. A
        # regression here means either a hand table entry drifted from gl.xml, or gl.xml
        # itself changed underneath an existing entry - either way, worth a human look, not a
        # silent pass.
        commands = load_all_commands()
        mismatches = find_pointer_length_mismatches(commands)
        self.assertEqual(mismatches, [], msg='\n'.join(mismatches))

    def test_renamed_only_alias_keys_are_skipped_not_flagged(self) -> None:
        # 'wglSetPixelFormat'/'wglDescribePixelFormat' are interceptor-exported rename-only
        # keys with no raw-registry command of that name (see pointer_metadata.py's own
        # "Dual-registration" comment) - must be silently skipped, not reported as a mismatch
        # or a crash, since this function has nothing to do with the interceptor's renaming.
        commands = load_all_commands()
        # Should not raise, and the real 'SetPixelFormat'/'DescribePixelFormat' keys (which DO
        # exist in the raw registry) must still be checked - covered by the "no mismatches"
        # test above already comparing every table entry, renamed-alias or not.
        find_pointer_length_mismatches(commands)


if __name__ == '__main__':
    unittest.main()
