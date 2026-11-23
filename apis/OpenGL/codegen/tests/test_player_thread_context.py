# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import re
import unittest
from pathlib import Path

CODEGEN_DIR = Path(__file__).resolve().parents[1]
PLAYER_DIR = CODEGEN_DIR.parent / 'player'

SWITCH = 'ThreadContextService::Get().SwitchTo(command.m_ThreadId);'
RUN_BODY_START = re.compile(r'^void (\S+)Runner::Run\(\) \{\n(.*)$', re.MULTILINE)


class PlayerThreadContextTests(unittest.TestCase):
    """Every runner must switch to its recorded thread's context before anything else, since
    resolving its arguments can already call the driver (a bind's implicit glGen*)."""

    def assert_runs_switch_first(self, path: Path) -> int:
        source = path.read_text(encoding='utf-8')
        runs = RUN_BODY_START.findall(source)
        for name, first_line in runs:
            self.assertEqual(first_line.strip(), SWITCH,
                             f'{path.name}: {name}Runner::Run() does not switch threads first')
        return len(runs)

    def test_generated_runners_switch_first(self) -> None:
        self.assertEqual(
            self.assert_runs_switch_first(CODEGEN_DIR / 'templates' /
                                          'commandRunnersAuto.cpp.mako'), 1)

    def test_hand_written_runners_switch_first(self) -> None:
        runs = 0
        for path in sorted(PLAYER_DIR.glob('*.cpp')):
            if not path.stem.endswith('Auto'):
                runs += self.assert_runs_switch_first(path)
        self.assertGreater(runs, 0)


if __name__ == '__main__':
    unittest.main()
