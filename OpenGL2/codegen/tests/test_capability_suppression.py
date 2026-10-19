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

from capability_suppression import (
    ARGUMENT_REMAPS,
    BINARY_LOAD_SKIPS,
    RESULT_FILTERS,
    STATE_QUERIES,
    argument_remaps,
    binary_load_skips,
    result_filters,
    state_queries,
)
from intermediates import Api
from intermediates_creator import extract_commands_from_registry, get_commands_from_xml_data
from test_generator_recorder import extract_wrapper_body, render_commands
from test_pointer_metadata import load_registry


class CapabilitySuppressionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        commands = []
        for api in (Api.GL, Api.WGL):
            raw = extract_commands_from_registry(load_registry(api))
            commands += get_commands_from_xml_data({api: raw})[api]
        cls.commands = commands
        hooked = (STATE_QUERIES | BINARY_LOAD_SKIPS.keys() | ARGUMENT_REMAPS.keys() |
                  RESULT_FILTERS.keys() | {'glGetError'})
        cls.rendered = render_commands(
            [cmd for cmd in commands if cmd.name in hooked], frozenset(),
            state_queries=state_queries(commands),
            binary_load_skips=binary_load_skips(commands),
            argument_remaps=argument_remaps(commands),
            result_filters=result_filters(commands))

    def test_queries_go_to_the_driver_only_when_not_answered(self) -> None:
        for name in sorted(STATE_QUERIES):
            with self.subTest(command=name):
                body = extract_wrapper_body(self.rendered, name)
                self.assertIn('if (!record || (!command->m_Skip && '
                              '!AnswerStateQuery(pname, data))) {\n'
                              '    realFunction(pname, data);', body)
                self.assertIn('CreateCommandKey()', body)

    def test_skipped_loads_return_before_reserving_a_key(self) -> None:
        for name, condition in sorted(BINARY_LOAD_SKIPS.items()):
            with self.subTest(command=name):
                body = extract_wrapper_body(self.rendered, name)
                skip = body.find(f'if (record && {condition}) {{\n    return;')
                self.assertNotEqual(skip, -1)
                self.assertLess(skip, body.find('CreateCommandKey()'))

    def test_only_the_driver_gets_a_remapped_index(self) -> None:
        body = extract_wrapper_body(self.rendered, 'glGetStringi')
        self.assertIn('realFunction(name, record ? DriverExtensionIndex(name, index) : index);',
                      body)
        self.assertIn('command.emplace(static_cast<uint32_t>(GetCurrentThreadId()), name, index);',
                      body)

    def test_results_are_filtered_right_after_the_driver_call(self) -> None:
        for name, replacement in sorted(RESULT_FILTERS.items()):
            with self.subTest(command=name):
                body = extract_wrapper_body(self.rendered, name)
                self.assertIn('command->m_Return.Value : realFunction', body)
                self.assertIn(f'  if (record) {{\n    result = {replacement};\n  }}', body)

    def test_other_commands_are_not_hooked(self) -> None:
        body = extract_wrapper_body(self.rendered, 'glGetError')
        for hook in ('AnswerStateQuery', 'Skips', 'DriverExtensionIndex', 'HideExtensions'):
            self.assertNotIn(hook, body)


if __name__ == '__main__':
    unittest.main()
