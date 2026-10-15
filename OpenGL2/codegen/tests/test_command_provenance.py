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
from typing import Any

CODEGEN_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(CODEGEN_DIR))

from generator_interceptor import _get_synthetic_wgl_commands  # pyright: ignore[reportPrivateUsage]
from intermediates import Api, CmdType, Command, ReturnValue
from intermediates_creator import get_commands_from_xml_data


def raw_command(name: str) -> dict[str, Any]:
    return {
        'command_attribs': {'name': name},
        'return_value': {'type': 'void', 'attribs': {}},
        'params': [],
        'alias_attribs': {},
    }


class CommandProvenanceTests(unittest.TestCase):
    def test_registry_commands_store_their_api(self) -> None:
        raw_functions = {
            api: [raw_command(f'{api.value}_command')]
            for api in Api
        }

        commands = get_commands_from_xml_data(raw_functions)
        self.assertEqual(set(commands), set(Api))

        for api, api_commands in commands.items():
            self.assertEqual(len(api_commands), 1)
            self.assertEqual(api_commands[0].api, api)

    def test_synthetic_wgl_commands_store_wgl_provenance(self) -> None:
        source_names = (
            'ChoosePixelFormat',
            'DescribePixelFormat',
            'GetPixelFormat',
            'SetPixelFormat',
            'SwapBuffers',
        )
        source_commands = [
            Command(
                name=name,
                api=Api.WGL,
                enabled=False,
                command_type=CmdType.NONE,
                return_value=ReturnValue(type='void'),
                params=[],
            )
            for name in source_names
        ]

        synthetic = _get_synthetic_wgl_commands(source_commands)
        expected_synthetic_names = {
            'wglChoosePixelFormat',
            'wglDescribePixelFormat',
            'wglGetPixelFormat',
            'wglSetPixelFormat',
            'wglSwapBuffers',
            'wglSwapMultipleBuffers',
        }

        self.assertEqual({command.api for command in synthetic}, {Api.WGL})
        self.assertEqual(
            {command.name for command in synthetic},
            expected_synthetic_names,
        )


if __name__ == '__main__':
    unittest.main()