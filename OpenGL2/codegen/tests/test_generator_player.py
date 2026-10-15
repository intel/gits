# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

"""Tests for the subcapture command sets generator_player.py derives."""

import sys
import unittest
from pathlib import Path

CODEGEN_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(CODEGEN_DIR))

from generator_player import analyzed_object_params, display_list_commands
from intermediates import Api
from intermediates_creator import extract_commands_from_registry, get_commands_from_xml_data
from test_intermediates_creator import load_registry

NAMES = (
    'glBindTexture', 'glBindTextures', 'glGenTextures', 'glDeleteTextures', 'glCreateProgram',
    'glAttachShader', 'glNewList', 'glEndList', 'glClear', 'wglMakeCurrent', 'wglSwapBuffers',
    'glBufferSubData', 'glNamedBufferData', 'glUnmapBuffer', 'glLinkProgram', 'glGenerateMipmap',
    'glVertexPointer', 'glPixelStorei', 'glBindBuffer', 'glTexImage2D', 'glUseProgram',
    'glTexStorage2D', 'glTextureStorage2D', 'glTexBuffer', 'glTextureBufferRange',
    'glFramebufferTexture2D', 'glNamedFramebufferRenderbuffer', 'glBlitFramebuffer',
    'glProgramUniform1f', 'glBindFramebuffer',
)


def get_commands():
    raw = [c for c in extract_commands_from_registry(load_registry(Api.GL))
           if c['command_attribs']['name'] in NAMES]
    raw += [c for c in extract_commands_from_registry(load_registry(Api.WGL))
            if c['command_attribs']['name'] in NAMES]
    commands = get_commands_from_xml_data({Api.GL: raw})[Api.GL]
    return {c.name: c for c in commands}


class GeneratorPlayerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.commands = get_commands()

    def test_display_lists_leave_out_commands_changing_restored_objects(self) -> None:
        listed = display_list_commands(list(self.commands.values()))
        for name in ('glGenTextures', 'glCreateProgram', 'glDeleteTextures', 'wglMakeCurrent',
                     'glBufferSubData', 'glNamedBufferData', 'glUnmapBuffer', 'glAttachShader',
                     'glLinkProgram', 'glGenerateMipmap', 'glTexStorage2D', 'glTextureStorage2D',
                     'glTexBuffer', 'glTextureBufferRange', 'glFramebufferTexture2D',
                     'glNamedFramebufferRenderbuffer', 'glBlitFramebuffer'):
            with self.subTest(command=name):
                self.assertNotIn(name, listed)

    def test_display_lists_keep_compiled_and_context_state_commands(self) -> None:
        listed = display_list_commands(list(self.commands.values()))
        for name in ('glBindTexture', 'glClear', 'glTexImage2D', 'glUseProgram',
                     'glVertexPointer', 'glPixelStorei', 'glBindBuffer', 'glProgramUniform1f',
                     'glBindFramebuffer'):
            with self.subTest(command=name):
                self.assertIn(name, listed)

    def test_analysis_collects_objects_commands_name(self) -> None:
        analyzed = analyzed_object_params(list(self.commands.values()))
        self.assertEqual(analyzed['glBindTexture'], ['texture'])
        self.assertEqual(analyzed['glBindTextures'], ['textures'])
        self.assertEqual(analyzed['glDeleteTextures'], ['textures'])
        self.assertEqual(analyzed['glAttachShader'], ['program', 'shader'])

    def test_analysis_skips_created_objects_and_custom_commands(self) -> None:
        analyzed = analyzed_object_params(list(self.commands.values()))
        for name in ('glGenTextures', 'glCreateProgram', 'glNewList', 'glEndList',
                     'wglSwapBuffers', 'glClear'):
            with self.subTest(command=name):
                self.assertNotIn(name, analyzed)


if __name__ == '__main__':
    unittest.main()
