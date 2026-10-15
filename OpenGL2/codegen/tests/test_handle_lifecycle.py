# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Covers handle_lifecycle.py's generic mapped-handle mechanism - the merged module that used to
# be two separate, structurally-identical files (this module's own HGLRC tables, plus a
# now-deleted sync_lifecycle.py for GLsync). Every test here is written against
# MAPPED_HANDLE_LIFECYCLES/its derived views generically (looping over every entry), not against
# HGLRC/GLsync by name, wherever that is possible - the whole point of the merge is that a third
# entry should not need a fourth parallel test file either.

import sys
import unittest
from pathlib import Path

CODEGEN_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(CODEGEN_DIR))

from handle_lifecycle import (
    CONTEXT_SHARE_PARAM,
    FRAME_END_COMMANDS,
    GLSYNC_LIFECYCLE,
    HANDLE_CREATING_COMMANDS,
    HANDLE_PARAM_TO_ERASE_ON_SUCCESS,
    HGLRC_LIFECYCLE,
    HPBUFFERARB_LIFECYCLE,
    HPBUFFEREXT_LIFECYCLE,
    MAPPED_HANDLE_LIFECYCLES,
    MAPPED_HANDLE_SERVICE_NAMES,
    SHARE_LISTS_COMMANDS,
    MappedHandleLifecycle,
)


class MappedHandleLifecyclesTableTests(unittest.TestCase):
    """Structural invariants every entry in MAPPED_HANDLE_LIFECYCLES must satisfy - written
    generically so they keep holding, with no test-file changes needed, if a third mapped-
    handle type is ever added."""

    def test_every_entry_is_a_mapped_handle_lifecycle(self) -> None:
        for entry in MAPPED_HANDLE_LIFECYCLES:
            with self.subTest(cpp_type=entry.cpp_type):
                self.assertIsInstance(entry, MappedHandleLifecycle)

    def test_cpp_types_are_unique(self) -> None:
        cpp_types = [entry.cpp_type for entry in MAPPED_HANDLE_LIFECYCLES]
        self.assertEqual(len(cpp_types), len(set(cpp_types)))

    def test_service_names_are_unique(self) -> None:
        service_names = [entry.service_name for entry in MAPPED_HANDLE_LIFECYCLES]
        self.assertEqual(len(service_names), len(set(service_names)))

    def test_no_command_creates_or_erases_two_different_handle_types(self) -> None:
        # HANDLE_CREATING_COMMANDS/HANDLE_PARAM_TO_ERASE_ON_SUCCESS union every entry's own
        # tables blindly (see handle_lifecycle.py's own comment on why that is safe) - verify
        # the safety condition itself here, generically, rather than merely trusting it.
        seen_creating: dict[str, str] = {}
        for entry in MAPPED_HANDLE_LIFECYCLES:
            for command_name in entry.creating_commands:
                self.assertNotIn(
                    command_name, seen_creating,
                    f'{command_name} mints both {seen_creating.get(command_name)} and '
                    f'{entry.cpp_type} - HANDLE_CREATING_COMMANDS union is unsafe')
                seen_creating[command_name] = entry.cpp_type

        seen_erasing: dict[str, str] = {}
        for entry in MAPPED_HANDLE_LIFECYCLES:
            for command_name in entry.param_to_erase_on_success:
                self.assertNotIn(
                    command_name, seen_erasing,
                    f'{command_name} erases both {seen_erasing.get(command_name)} and '
                    f'{entry.cpp_type} - HANDLE_PARAM_TO_ERASE_ON_SUCCESS union is unsafe')
                seen_erasing[command_name] = entry.cpp_type

    def test_each_entry_has_at_least_one_creating_command(self) -> None:
        for entry in MAPPED_HANDLE_LIFECYCLES:
            with self.subTest(cpp_type=entry.cpp_type):
                self.assertTrue(entry.creating_commands)


class DerivedViewsTests(unittest.TestCase):
    """HANDLE_CREATING_COMMANDS / HANDLE_PARAM_TO_ERASE_ON_SUCCESS / MAPPED_HANDLE_SERVICE_NAMES
    are pre-unioned derived views over MAPPED_HANDLE_LIFECYCLES - check they actually match what
    a fresh union of every entry would produce (catches a future entry being added to
    MAPPED_HANDLE_LIFECYCLES but forgotten by one of the derived views, or vice versa)."""

    def test_handle_creating_commands_is_the_union_of_every_entry(self) -> None:
        expected: frozenset[str] = frozenset()
        for entry in MAPPED_HANDLE_LIFECYCLES:
            expected |= entry.creating_commands
        self.assertEqual(HANDLE_CREATING_COMMANDS, expected)

    def test_handle_param_to_erase_on_success_is_the_union_of_every_entry(self) -> None:
        expected: dict[str, str] = {}
        for entry in MAPPED_HANDLE_LIFECYCLES:
            expected.update(entry.param_to_erase_on_success)
        self.assertEqual(HANDLE_PARAM_TO_ERASE_ON_SUCCESS, expected)

    def test_mapped_handle_service_names_covers_every_entrys_cpp_type(self) -> None:
        for entry in MAPPED_HANDLE_LIFECYCLES:
            with self.subTest(cpp_type=entry.cpp_type):
                self.assertEqual(MAPPED_HANDLE_SERVICE_NAMES[entry.cpp_type], entry.service_name)
        self.assertEqual(len(MAPPED_HANDLE_SERVICE_NAMES), len(MAPPED_HANDLE_LIFECYCLES))


class HglrcLifecycleTests(unittest.TestCase):
    """HGLRC's own entry - values must stay exactly what the pre-merge handle_lifecycle.py
    hand-maintained as its own top-level HANDLE_CREATING_COMMANDS/HANDLE_PARAM_TO_ERASE_ON_
    SUCCESS constants, since this is a pure refactor with no intended behavior change."""

    def test_cpp_type_and_service_name(self) -> None:
        self.assertEqual(HGLRC_LIFECYCLE.cpp_type, 'HGLRC')
        self.assertEqual(HGLRC_LIFECYCLE.service_name, 'ContextHandleMapService')

    def test_creating_commands(self) -> None:
        self.assertEqual(
            HGLRC_LIFECYCLE.creating_commands,
            frozenset({'wglCreateContext', 'wglCreateContextAttribsARB'}))

    def test_param_to_erase_on_success(self) -> None:
        self.assertEqual(
            HGLRC_LIFECYCLE.param_to_erase_on_success, {'wglDeleteContext': 'oldContext'})


class GlsyncLifecycleTests(unittest.TestCase):
    """GLsync's own entry - values must stay exactly what the pre-merge sync_lifecycle.py
    hand-maintained as SYNC_HANDLE_CREATING_COMMANDS/SYNC_HANDLE_PARAM_TO_ERASE_ON_SUCCESS,
    since this is a pure refactor with no intended behavior change."""

    def test_cpp_type_and_service_name(self) -> None:
        self.assertEqual(GLSYNC_LIFECYCLE.cpp_type, 'GLsync')
        self.assertEqual(GLSYNC_LIFECYCLE.service_name, 'SyncHandleMapService')

    def test_creating_commands(self) -> None:
        self.assertEqual(
            GLSYNC_LIFECYCLE.creating_commands, frozenset({'glFenceSync', 'glFenceSyncAPPLE'}))

    def test_param_to_erase_on_success(self) -> None:
        self.assertEqual(
            GLSYNC_LIFECYCLE.param_to_erase_on_success,
            {'glDeleteSync': 'sync', 'glDeleteSyncAPPLE': 'sync'})


class HpbufferarbLifecycleTests(unittest.TestCase):
    def test_cpp_type_and_service_name(self) -> None:
        self.assertEqual(HPBUFFERARB_LIFECYCLE.cpp_type, 'HPBUFFERARB')
        self.assertEqual(HPBUFFERARB_LIFECYCLE.service_name, 'PbufferHandleMapService')

    def test_creating_commands(self) -> None:
        self.assertEqual(HPBUFFERARB_LIFECYCLE.creating_commands,
                         frozenset({'wglCreatePbufferARB'}))

    def test_param_to_erase_on_success(self) -> None:
        self.assertEqual(HPBUFFERARB_LIFECYCLE.param_to_erase_on_success,
                         {'wglDestroyPbufferARB': 'hPbuffer'})


class HpbufferextLifecycleTests(unittest.TestCase):
    def test_cpp_type_and_service_name(self) -> None:
        self.assertEqual(HPBUFFEREXT_LIFECYCLE.cpp_type, 'HPBUFFEREXT')
        self.assertEqual(HPBUFFEREXT_LIFECYCLE.service_name, 'PbufferEXTHandleMapService')

    def test_creating_commands(self) -> None:
        self.assertEqual(HPBUFFEREXT_LIFECYCLE.creating_commands,
                         frozenset({'wglCreatePbufferEXT'}))

    def test_param_to_erase_on_success(self) -> None:
        self.assertEqual(HPBUFFEREXT_LIFECYCLE.param_to_erase_on_success,
                         {'wglDestroyPbufferEXT': 'hPbuffer'})


class ContextShareTablesTests(unittest.TestCase):
    """CONTEXT_SHARE_PARAM/SHARE_LISTS_COMMANDS stay HGLRC-only, hand-maintained tables outside
    the generic MAPPED_HANDLE_LIFECYCLES mechanism (context sharing has no GLsync
    counterpart) - unchanged by the merge, so still exactly their pre-merge values."""

    def test_context_share_param(self) -> None:
        self.assertEqual(
            CONTEXT_SHARE_PARAM,
            {'wglCreateContext': None, 'wglCreateContextAttribsARB': 'hShareContext'})

    def test_every_context_share_param_key_is_a_creating_command(self) -> None:
        for command_name in CONTEXT_SHARE_PARAM:
            with self.subTest(command=command_name):
                self.assertIn(command_name, HGLRC_LIFECYCLE.creating_commands)

    def test_share_lists_commands(self) -> None:
        self.assertEqual(
            SHARE_LISTS_COMMANDS, {'wglShareLists': ('hrcSrvShare', 'hrcSrvSource')})

    def test_frame_end_commands(self) -> None:
        self.assertEqual(FRAME_END_COMMANDS, frozenset({'wglSwapBuffers', 'wglSwapLayerBuffers'}))


class HypotheticalThirdHandleTypeTests(unittest.TestCase):
    """Sanity-checks the refactor's own central claim - that a third mapped-handle type only
    needs one new MAPPED_HANDLE_LIFECYCLES entry, with zero changes to any derived view's
    *code* - by actually constructing one and re-deriving each view the same way
    handle_lifecycle.py's module-level code does, entirely inside this test (never mutating the
    real module-level tables)."""

    def test_adding_a_third_entry_re_derives_every_view_correctly(self) -> None:
        hypothetical = MappedHandleLifecycle(
            cpp_type='GLDisplayList',
            service_name='DisplayListHandleMapService',
            creating_commands=frozenset({'glGenLists'}),
            param_to_erase_on_success={'glDeleteLists': 'list'},
        )
        entries = MAPPED_HANDLE_LIFECYCLES + (hypothetical,)

        rederived_creating_commands: frozenset[str] = frozenset().union(
            *(entry.creating_commands for entry in entries))
        rederived_param_to_erase: dict[str, str] = {
            command_name: param_name
            for entry in entries
            for command_name, param_name in entry.param_to_erase_on_success.items()
        }
        rederived_service_names: dict[str, str] = {
            entry.cpp_type: entry.service_name for entry in entries
        }

        self.assertEqual(
            rederived_creating_commands, HANDLE_CREATING_COMMANDS | {'glGenLists'})
        self.assertEqual(
            rederived_param_to_erase,
            {**HANDLE_PARAM_TO_ERASE_ON_SUCCESS, 'glDeleteLists': 'list'})
        self.assertEqual(
            rederived_service_names,
            {**MAPPED_HANDLE_SERVICE_NAMES, 'GLDisplayList': 'DisplayListHandleMapService'})

        # The real, unmodified module-level tables are untouched by this test.
        self.assertNotIn('glGenLists', HANDLE_CREATING_COMMANDS)
        self.assertNotIn('GLDisplayList', MAPPED_HANDLE_SERVICE_NAMES)


if __name__ == '__main__':
    unittest.main()
