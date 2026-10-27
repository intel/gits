# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import json
import sys
import tempfile
import unittest
from pathlib import Path

CODEGEN_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(CODEGEN_DIR))

from command_ids import ID_GL_BEGIN, ID_WGL_BEGIN, build_command_ids
from intermediates import Api, CmdType, Command, ReturnValue


def fake_cmd(name: str, *, api: Api = Api.GL, enabled: bool) -> Command:
    return Command(
        name=name,
        api=api,
        enabled=enabled,
        command_type=CmdType.NONE,
        return_value=ReturnValue(type='void'),
        params=[],
    )


def empty_allocations() -> dict[str, object]:
    return {
        'command_count': {api.value: 0 for api in Api},
        'command_ids': {api.value: {} for api in Api},
    }


class CommandIdTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary_directory.cleanup)
        self.allocations_path = Path(self.temporary_directory.name) / 'command_ids.json'
        self.write_allocations(empty_allocations())

    def write_allocations(self, allocations: dict[str, object]) -> None:
        self.allocations_path.write_text(json.dumps(allocations), encoding='utf-8')

    def read_allocations(self) -> dict[str, object]:
        return json.loads(self.allocations_path.read_text(encoding='utf-8'))

    def test_disabled_commands_receive_no_id(self) -> None:
        command_ids = build_command_ids([fake_cmd('glClear', enabled=False)], self.allocations_path)

        self.assertEqual(command_ids, {api.value: {} for api in Api})
        self.assertEqual(self.read_allocations(), empty_allocations())

    def test_existing_ids_survive_reordering_and_removed_commands(self) -> None:
        build_command_ids(
            [fake_cmd('glClear', enabled=True), fake_cmd('glFlush', enabled=True)],
            self.allocations_path,
        )
        command_ids = build_command_ids(
            [fake_cmd('glFinish', enabled=True), fake_cmd('glClear', enabled=True)],
            self.allocations_path,
        )

        self.assertEqual(command_ids['GL'], {
            'ID_GLCLEAR': ID_GL_BEGIN,
            'ID_GLFLUSH': ID_GL_BEGIN + 1,
            'ID_GLFINISH': ID_GL_BEGIN + 2,
        })

    def test_apis_get_independent_id_ranges(self) -> None:
        # Each API keeps its own append-only counter starting at its own reserved base (see
        # command_ids.py's _API_RANGE_BEGIN) - a WGL command must not consume a GL-range ID
        # (or vice versa), unlike a single flat namespace where insertion order alone would
        # determine the next free slot regardless of which API a command belongs to.
        command_ids = build_command_ids(
            [
                fake_cmd('glClear', api=Api.GL, enabled=True),
                fake_cmd('wglCreateContext', api=Api.WGL, enabled=True),
                fake_cmd('glFlush', api=Api.GL, enabled=True),
            ],
            self.allocations_path,
        )

        self.assertEqual(command_ids['GL'], {
            'ID_GLCLEAR': ID_GL_BEGIN,
            'ID_GLFLUSH': ID_GL_BEGIN + 1,
        })
        self.assertEqual(command_ids['WGL'], {
            'ID_WGLCREATECONTEXT': ID_WGL_BEGIN,
        })

    def test_rejects_malformed_allocations_file(self) -> None:
        self.write_allocations({'command_count': {}, 'command_ids': {}})

        with self.assertRaises(ValueError):
            build_command_ids([fake_cmd('glClear', enabled=True)], self.allocations_path)


if __name__ == '__main__':
    unittest.main()
