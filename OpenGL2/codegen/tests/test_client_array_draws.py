#!/usr/bin/python

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

from client_array_draws import client_array_draw_for, client_array_draws, client_array_setters
from generator_helpers import pointer_override_for
from generator_recorder import PRE_HOOK_KEY_PAIR_COMMANDS, pre_hook_key_pair_commands
from intermediates import Api
from intermediates_creator import extract_commands_from_registry, get_commands_from_xml_data
from pointer_metadata import LengthSourceKind
from test_pointer_metadata import load_registry


class ClientArrayDrawsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        raw = extract_commands_from_registry(load_registry(Api.GL))
        cls.commands = get_commands_from_xml_data({Api.GL: raw})[Api.GL]
        cls.by_name = {cmd.name: cmd for cmd in cls.commands}
        cls.draws = client_array_draws(cls.commands)

    def test_single_draws_are_hooked(self) -> None:
        for name in (
            'glDrawArrays', 'glDrawArraysInstanced', 'glDrawArraysInstancedBaseInstance',
            'glDrawElements', 'glDrawElementsBaseVertex', 'glDrawElementsInstanced',
            'glDrawElementsInstancedBaseVertexBaseInstance', 'glDrawRangeElements',
            'glDrawRangeElementsBaseVertex',
        ):
            with self.subTest(command=name):
                self.assertIn(name, self.draws)

    def test_parameter_roles(self) -> None:
        draw = self.draws['glDrawElementsInstancedBaseVertexBaseInstance']
        self.assertTrue(draw.is_elements)
        self.assertIsNone(draw.first_param)
        self.assertEqual(draw.base_vertex_param, 'basevertex')
        self.assertEqual(draw.instance_count_param, 'instancecount')
        self.assertEqual(draw.base_instance_param, 'baseinstance')
        arrays = self.draws['glDrawArrays']
        self.assertFalse(arrays.is_elements)
        self.assertEqual(arrays.first_param, 'first')
        self.assertIsNone(arrays.instance_count_param)
        self.assertEqual(self.draws['glDrawArraysInstancedARB'].instance_count_param, 'primcount')

    def test_multi_draws_are_hooked(self) -> None:
        for name, count_param, is_elements in (
            ('glMultiDrawArrays', 'drawcount', False),
            ('glMultiDrawArraysEXT', 'primcount', False),
            ('glMultiDrawElementsBaseVertex', 'drawcount', True),
            ('glMultiDrawElementsBaseVertexEXT', 'drawcount', True),
        ):
            with self.subTest(command=name):
                draw = self.draws[name]
                self.assertEqual(draw.draw_count_param, count_param)
                self.assertEqual(draw.is_elements, is_elements)
                self.assertIsNone(draw.instance_count_param)
        self.assertEqual(self.draws['glDrawArraysInstancedEXT'].first_param, 'start')

    def test_indirect_and_other_draws_are_not_hooked(self) -> None:
        for name in (
            'glDrawArraysIndirect', 'glMultiDrawArraysIndirect', 'glDrawElementsIndirect',
            'glDrawTransformFeedback', 'glDrawElementArrayAPPLE', 'glMultiDrawElementArrayAPPLE',
            'glDrawPixels',
        ):
            with self.subTest(command=name):
                self.assertIsNone(client_array_draw_for(self.by_name[name]))

    def test_hooked_element_draws_capture_indices(self) -> None:
        # The generated hook reads m_indices and m_indicesRawPointer.
        for name, draw in self.draws.items():
            if draw.is_elements and draw.draw_count_param is None:
                with self.subTest(command=name):
                    override = pointer_override_for(self.by_name[name], 'indices')
                    self.assertEqual(override.length_source_kind,
                                     LengthSourceKind.ELEMENT_INDICES_BYTE_SIZE)

    def test_pointer_setters(self) -> None:
        setters = client_array_setters(self.commands)
        expected = {
            'glVertexPointer': ('Vertex', '0'),
            'glColorPointerEXT': ('Color', '0'),
            'glFogCoordPointer': ('FogCoord', '0'),
            'glTexCoordPointer': ('TexCoord', '-1'),
            'glMultiTexCoordPointerEXT': ('TexCoord', 'static_cast<int64_t>(texunit) - GL_TEXTURE0'),
            'glVertexAttribPointer': ('VertexAttrib', 'static_cast<int64_t>(index)'),
            'glVertexAttribIPointer': ('VertexAttrib', 'static_cast<int64_t>(index)'),
            'glVertexAttribLPointerEXT': ('VertexAttrib', 'static_cast<int64_t>(index)'),
            'glInterleavedArrays': ('Interleaved', '-1'),
        }
        for name, (kind, slot_expr) in expected.items():
            with self.subTest(command=name):
                self.assertEqual((setters[name].kind, setters[name].slot_expr), (kind, slot_expr))
        # Setters for arrays the service never captures, and getters.
        for name in ('glVertexAttribPointerNV', 'glGetPointerv', 'glTangentPointerEXT',
                     'glVertexArrayVertexAttribOffsetEXT'):
            with self.subTest(command=name):
                self.assertNotIn(name, setters)

    def test_disabled_setters_are_reported(self) -> None:
        disabled = [cmd for cmd in self.commands if not cmd.enabled]
        self.assertTrue(client_array_setters(disabled))

    def test_hooked_draws_reserve_key_pairs(self) -> None:
        pairs = pre_hook_key_pair_commands(self.commands)
        self.assertEqual(pairs, PRE_HOOK_KEY_PAIR_COMMANDS | frozenset(self.draws))
        self.assertFalse(PRE_HOOK_KEY_PAIR_COMMANDS & frozenset(self.draws))


if __name__ == '__main__':
    unittest.main()
