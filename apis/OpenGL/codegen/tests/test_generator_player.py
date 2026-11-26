# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

"""Tests for the subcapture command sets generator_player.py derives and the command runner
parts it generates."""

import re
import sys
import unittest
from pathlib import Path

CODEGEN_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(CODEGEN_DIR))

from custom_player_runners import UNREPLAYED_COMMANDS
from generator_helpers import PLATFORM_GUARDS, mako_render_parts
from generator_player import COMMAND_RUNNER_PARTS, analyzed_object_params, display_list_commands
from handle_lifecycle import HANDLE_CREATING_COMMANDS, SHARE_LISTS_COMMANDS
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


class CommandRunnerPartsTests(unittest.TestCase):
    """The command runners are generated into COMMAND_RUNNER_PARTS files, which share the
    discarded output buffers defined in part 0."""

    TEMPLATE = str(CODEGEN_DIR / 'templates' / 'commandRunnersAuto.cpp.mako')
    SCRATCH = 'void* DiscardedOutputScratchBytes(size_t slot, size_t bytes)'

    @classmethod
    def setUpClass(cls) -> None:
        cls.commands = list(get_commands().values())
        for command in cls.commands:
            command.enabled = True
        cls.parts = cls.render(cls.commands)

    @classmethod
    def render(cls, commands) -> list[str]:
        return mako_render_parts(cls.TEMPLATE, COMMAND_RUNNER_PARTS, commands,
                                 handle_creating_commands=HANDLE_CREATING_COMMANDS,
                                 share_lists_commands=SHARE_LISTS_COMMANDS,
                                 unreplayed_commands=UNREPLAYED_COMMANDS,
                                 platform_guards=PLATFORM_GUARDS)

    def test_every_runner_is_defined_exactly_once_in_command_order(self) -> None:
        defined = [re.findall(r'^void (\w+)Runner::Run\(\) \{$', part, re.MULTILINE)
                   for part in self.parts]
        self.assertEqual([name for names in defined for name in names],
                         [command.name for command in self.commands])
        sizes = [len(names) for names in defined]
        self.assertLessEqual(max(sizes) - min(sizes), 1)

    def test_only_part_0_defines_the_discarded_output_buffers(self) -> None:
        self.assertEqual([part.count(self.SCRATCH + ' {') for part in self.parts],
                         [1] + [0] * (COMMAND_RUNNER_PARTS - 1))
        self.assertEqual([part.count(self.SCRATCH + ';') for part in self.parts],
                         [0] + [1] * (COMMAND_RUNNER_PARTS - 1))
        self.assertEqual([part.count('thread_local std::vector') for part in self.parts],
                         [1] + [0] * (COMMAND_RUNNER_PARTS - 1))

    def test_a_part_without_commands_is_rejected(self) -> None:
        with self.assertRaises(ValueError):
            self.render(self.commands[:COMMAND_RUNNER_PARTS - 1])

    def test_the_build_lists_every_part(self) -> None:
        expected = [f'commandRunners{index}Auto.cpp' for index in range(COMMAND_RUNNER_PARTS)]
        for cmake_lists in (CODEGEN_DIR / 'CMakeLists.txt',
                            CODEGEN_DIR.parent / 'player' / 'CMakeLists.txt'):
            with self.subTest(file=str(cmake_lists)):
                self.assertEqual(
                    re.findall(r'commandRunners\d+Auto\.cpp', cmake_lists.read_text()), expected)


if __name__ == '__main__':
    unittest.main()
