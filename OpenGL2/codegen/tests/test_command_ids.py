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

from command_ids import ID_OPENGL_BEGIN, build_command_ids
from intermediates import Api, CmdType, Command, ReturnValue


def fake_cmd(name: str, *, enabled: bool) -> Command:
    return Command(
        name=name,
        api=Api.GL,
        enabled=enabled,
        command_type=CmdType.NONE,
        return_value=ReturnValue(type='void'),
        params=[],
    )


class CommandIdTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary_directory.cleanup)
        self.allocations_path = Path(self.temporary_directory.name) / 'command_ids.json'
        self.write_allocations({'command_count': 0, 'command_ids': {}})

    def write_allocations(self, allocations: dict[str, object]) -> None:
        self.allocations_path.write_text(json.dumps(allocations), encoding='utf-8')

    def read_allocations(self) -> dict[str, object]:
        return json.loads(self.allocations_path.read_text(encoding='utf-8'))

    def test_disabled_commands_receive_no_id(self) -> None:
        command_ids = build_command_ids([fake_cmd('glClear', enabled=False)], self.allocations_path)

        self.assertEqual(command_ids, {})
        self.assertEqual(self.read_allocations(), {'command_count': 0, 'command_ids': {}})

    def test_existing_ids_survive_reordering_and_removed_commands(self) -> None:
        build_command_ids(
            [fake_cmd('glClear', enabled=True), fake_cmd('glFlush', enabled=True)],
            self.allocations_path,
        )
        command_ids = build_command_ids(
            [fake_cmd('glFinish', enabled=True), fake_cmd('glClear', enabled=True)],
            self.allocations_path,
        )

        self.assertEqual(command_ids, {
            'ID_GLCLEAR': ID_OPENGL_BEGIN,
            'ID_GLFLUSH': ID_OPENGL_BEGIN + 1,
            'ID_GLFINISH': ID_OPENGL_BEGIN + 2,
        })

if __name__ == '__main__':
    unittest.main()