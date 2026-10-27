# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import sys
import unittest
from dataclasses import replace
from pathlib import Path

CODEGEN_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(CODEGEN_DIR))

from intermediates import Api, Parameter
from intermediates_creator import FILE_TO_API, load_registries_and_extract_xml_data
from pname_value_counts import (
    PNAME_GROUPS,
    derive_pname_value_count_override,
    multi_value_pname_cases,
    validate_pname_groups,
)
from pointer_metadata import LengthSourceKind, PointerDirection

_ENUMS = None


def _enums():
    global _ENUMS
    if _ENUMS is None:
        _ENUMS, _ = load_registries_and_extract_xml_data(FILE_TO_API)
    return _ENUMS


class PnameGroupAuditTests(unittest.TestCase):
    def test_audit_matches_registry(self) -> None:
        validate_pname_groups(_enums())

    def test_group_membership_change_fails_generation(self) -> None:
        enums = dict(_enums())
        enums[Api.GL] = [e for e in enums[Api.GL] if e.name != 'GL_SPOT_EXPONENT']
        with self.assertRaisesRegex(ValueError, 'LightParameter'):
            validate_pname_groups(enums)

    def test_cases_cover_light_and_fog_values(self) -> None:
        cases = {names[0]: count for _, count, names in multi_value_pname_cases(_enums())}
        self.assertEqual(cases.get('GL_POSITION'), 4)
        self.assertEqual(cases.get('GL_SPOT_DIRECTION'), 3)
        self.assertEqual(cases.get('GL_FOG_COLOR'), 4)


class DerivePnameValueCountTests(unittest.TestCase):
    _PNAME = Parameter(name='pname', type='GLenum', group='LightParameter')
    _PARAMS = Parameter(name='params', type='const GLfloat *', len='COMPSIZE(pname)')

    def test_audited_group_setter_is_derived(self) -> None:
        override = derive_pname_value_count_override('glLightfv', self._PARAMS, [self._PNAME])
        self.assertIsNotNone(override)
        self.assertEqual(override.direction, PointerDirection.IN)
        self.assertEqual(override.length_source_kind, LengthSourceKind.PNAME_VALUE_COUNT)
        self.assertEqual(override.pname_param, 'pname')

    def test_unaudited_group_is_not_derived(self) -> None:
        pname = replace(self._PNAME, group='GetPName')
        self.assertNotIn('GetPName', PNAME_GROUPS)
        self.assertIsNone(derive_pname_value_count_override('glFoo', self._PARAMS, [pname]))

    def test_non_const_pointer_is_not_derived(self) -> None:
        params = replace(self._PARAMS, type='GLfloat *')
        self.assertIsNone(derive_pname_value_count_override('glFoo', params, [self._PNAME]))

    def test_void_pointer_is_not_derived(self) -> None:
        params = replace(self._PARAMS, type='const void *')
        self.assertIsNone(derive_pname_value_count_override('glFoo', params, [self._PNAME]))


if __name__ == '__main__':
    unittest.main()
