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

from generator_interceptor import (
    _make_wgl_swap_multiple_buffers,  # pyright: ignore[reportPrivateUsage]
)
from intermediates import (
    Api,
    TypeFacts,
)
from intermediates_creator import (
    FILE_TO_API,
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


def raw_command(name: str) -> dict[str, Any]:
    return {
        'command_attribs': {'name': name},
        'return_value': {'type': 'void', 'attribs': {}},
        'params': [],
        'alias_attribs': {},
    }


class TypeFactsTests(unittest.TestCase):
    def test_registry_declarations_preserve_type_facts(self) -> None:
        cases = (
            (
                Api.EGL,
                'eglCreateWaylandBufferFromImageWL',
                'return_value',
                'struct wl_buffer *',
                TypeFacts(base_type='struct wl_buffer', pointer_depth=1),
            ),
            (
                Api.WGL,
                'wglChoosePixelFormatARB',
                'pfAttribFList',
                'const FLOAT *',
                TypeFacts(base_type='FLOAT', pointer_depth=1),
            ),
            (
                Api.GLX,
                'glXAssociateDMPbufferSGIX',
                'dpy',
                'Display *',
                TypeFacts(base_type='Display', pointer_depth=1),
            ),
            (
                Api.GL,
                'glCompileShaderIncludeARB',
                'path',
                'const GLchar *const *',
                TypeFacts(base_type='GLchar', pointer_depth=2),
            ),
            (
                Api.GL,
                'glColorPointer',
                'pointer',
                'const void *',
                TypeFacts(base_type='void', pointer_depth=1),
            ),
            (
                Api.EGL,
                'eglCreatePixmapSurfaceHI',
                'pixmap',
                'struct EGLClientPixmapHI *',
                TypeFacts(base_type='struct EGLClientPixmapHI', pointer_depth=1),
            ),
        )

        for api, command_name, parameter_name, expected_type, expected_type_facts in cases:
            with self.subTest(command=command_name, parameter=parameter_name):
                registry = load_registry(api)
                raw_commands = extract_commands_from_registry(registry)
                raw = next(
                    command for command in raw_commands
                    if command['command_attribs']['name'] == command_name
                )
                raw_parameter = (
                    raw['return_value']
                    if parameter_name == 'return_value'
                    else next(param for param in raw['params'] if param['name'] == parameter_name)
                )

                self.assertEqual(raw_parameter['type'], expected_type)

                command = get_commands_from_xml_data({api: [raw]})[api][0]
                parameter = (
                    command.return_value
                    if parameter_name == 'return_value'
                    else next(param for param in command.params if param.name == parameter_name)
                )
                self.assertEqual(parameter.type, expected_type)
                self.assertEqual(parameter.type_facts, expected_type_facts)

    def test_parameter_len_expression_is_preserved(self) -> None:
        expected_type = 'const GLubyte *'
        expected_len = 'COMPSIZE(format,type)'

        raw = raw_command('glExample')
        raw['params'] = [{
            'name': 'data',
            'type': expected_type,
            'attribs': {'len': expected_len},
        }]

        command = get_commands_from_xml_data({Api.GL: [raw]})[Api.GL][0]

        self.assertEqual(command.params[0].len, expected_len)
        self.assertEqual(command.params[0].type, expected_type)
        self.assertEqual(
            command.params[0].type_facts,
            TypeFacts(base_type='GLubyte', pointer_depth=1),
        )

    def test_synthetic_wgl_parameters(self) -> None:
        command = _make_wgl_swap_multiple_buffers()

        self.assertEqual(
            [(param.name, param.type_facts) for param in command.params],
            [
                ('n', TypeFacts(base_type='UINT', pointer_depth=0)),
                ('pBuffers', TypeFacts(base_type='WGLSWAP', pointer_depth=1)),
            ],
        )
        self.assertEqual(command.params[1].type, 'const WGLSWAP*')

if __name__ == '__main__':
    unittest.main()