#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import re
import sys
import unittest
from pathlib import Path

CODEGEN_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(CODEGEN_DIR))

from buffer_content_hooks import _FAMILIES, buffer_content_hooks
from intermediates import Api
from intermediates_creator import extract_commands_from_registry, get_commands_from_xml_data
from test_pointer_metadata import load_registry


class BufferContentHooksTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        raw = extract_commands_from_registry(load_registry(Api.GL))
        cls.commands = get_commands_from_xml_data({Api.GL: raw})[Api.GL]
        cls.hooks = buffer_content_hooks(cls.commands)

    def test_representative_calls(self) -> None:
        expected = {
            'glBufferData':
                'OnData("glBufferData", BufferRef::ByTarget(target), '
                'static_cast<uint64_t>(size), data)',
            'glNamedBufferStorage':
                'OnData("glNamedBufferStorage", BufferRef::ByName(buffer), '
                'static_cast<uint64_t>(size), data)',
            'glBufferSubDataARB':
                'OnSubData(BufferRef::ByTarget(target), static_cast<uint64_t>(offset), '
                'static_cast<uint64_t>(size), data)',
            'glCopyBufferSubData':
                'OnGpuWrite("glCopyBufferSubData", BufferRef::ByTarget(writeTarget), '
                'static_cast<uint64_t>(writeOffset), static_cast<uint64_t>(size))',
            'glCopyNamedBufferSubData':
                'OnGpuWrite("glCopyNamedBufferSubData", BufferRef::ByName(writeBuffer), '
                'static_cast<uint64_t>(writeOffset), static_cast<uint64_t>(size))',
            'glNamedCopyBufferSubDataEXT':
                'OnGpuWrite("glNamedCopyBufferSubDataEXT", BufferRef::ByName(writeBuffer), '
                'static_cast<uint64_t>(writeOffset), static_cast<uint64_t>(size))',
            'glClearBufferSubData':
                'OnGpuWrite("glClearBufferSubData", BufferRef::ByTarget(target), '
                'static_cast<uint64_t>(offset), static_cast<uint64_t>(size))',
            'glClearNamedBufferSubDataEXT':
                'OnGpuWrite("glClearNamedBufferSubDataEXT", BufferRef::ByName(buffer), '
                'static_cast<uint64_t>(offset), static_cast<uint64_t>(size))',
            'glClearBufferData': 'OnGpuWrite("glClearBufferData", BufferRef::ByTarget(target))',
            'glClearNamedBufferData':
                'OnGpuWrite("glClearNamedBufferData", BufferRef::ByName(buffer))',
            'glTexBuffer': 'OnUnhookedGpuWriter("glTexBuffer", BufferRef::ByName(buffer))',
            'glTransformFeedbackBufferRange':
                'OnGpuResultWrite("glTransformFeedbackBufferRange", BufferRef::ByName(buffer))',
            'glGetQueryBufferObjectuiv':
                'OnGpuResultWrite("glGetQueryBufferObjectuiv", BufferRef::ByName(buffer))',
            'glBufferStorageMemEXT':
                'OnUnhookedGpuWriter("glBufferStorageMemEXT", BufferRef::ByTarget(target))',
            'glMakeNamedBufferResidentNV':
                'OnUnhookedGpuWriter("glMakeNamedBufferResidentNV", BufferRef::ByName(buffer))',
            'glInvalidateBufferSubData': 'OnUndefined(BufferRef::ByName(buffer))',
            'glMapNamedBufferRange': 'OnUndefined(BufferRef::ByName(buffer))',
            'glBindBufferARB': 'OnBind("glBindBufferARB", target, buffer)',
            'glBindBufferRangeNV': 'OnBindIndexed("glBindBufferRangeNV", target, buffer)',
            'glBindBuffersBase': 'OnBindIndexed("glBindBuffersBase", target, count, buffers)',
            'glDeleteBuffers': 'OnDelete(n, buffers)',
        }
        for name, call in expected.items():
            with self.subTest(command=name):
                self.assertEqual(self.hooks[name].call, call)

    def test_sizes_and_offsets_are_guarded(self) -> None:
        expected = {
            'glBufferData': 'size >= 0',
            'glNamedBufferStorage': 'size >= 0',
            'glBufferSubDataARB': 'offset >= 0 && size >= 0',
            'glCopyBufferSubData': 'writeOffset >= 0 && size >= 0',
            'glClearNamedBufferSubData': 'offset >= 0 && size >= 0',
            'glClearBufferData': None,
            'glTexBuffer': None,
        }
        for name, guard in expected.items():
            with self.subTest(command=name):
                self.assertEqual(self.hooks[name].guard, guard)

    def test_copies_and_clears_pass_the_range_they_write(self) -> None:
        # The recorder reads the written range back into a persistent mapping's shadow; only a
        # whole-buffer clear has no range, and gets the whole mapping read back.
        for cmd in self.commands:
            if not re.match(r'^gl(Copy|CopyNamed|NamedCopy|Clear(Named)?)Buffer(Sub)?Data(EXT)?$',
                            cmd.name):
                continue
            with self.subTest(command=cmd.name):
                call = self.hooks[cmd.name].call
                self.assertTrue(call.startswith(f'OnGpuWrite("{cmd.name}", '), call)
                prepare = self.hooks[cmd.name].prepare
                self.assertTrue(prepare.startswith('BeforeGpuWrite('), prepare)
                whole_clear = cmd.name.startswith('glClear') and 'SubData' not in cmd.name
                self.assertEqual(call.count('static_cast<uint64_t>'), 0 if whole_clear else 2,
                                 call)
                self.assertEqual(prepare.count('static_cast<uint64_t>'),
                                 0 if whole_clear else 2, prepare)

    def test_guarded_parameters_are_signed(self) -> None:
        # `x >= 0` only rejects anything for a signed type.
        signed = {'GLsizeiptr', 'GLintptr', 'GLsizeiptrARB', 'GLintptrARB', 'GLsizei', 'GLint'}
        for cmd in self.commands:
            hook = self.hooks.get(cmd.name)
            if hook is None or hook.guard is None:
                continue
            types = {p.name: p.type for p in cmd.params}
            for term in hook.guard.split(' && '):
                with self.subTest(command=cmd.name, term=term):
                    self.assertIn(types[term.removesuffix(' >= 0')], signed)

    def test_query_result_writes_are_hooked_before_the_call(self) -> None:
        expected = {
            'glGetQueryBufferObjectiv': True,
            'glGetQueryBufferObjectui64v': True,
            'glTransformFeedbackBufferBase': False,
            'glBindBufferBase': False,
            'glCopyBufferSubData': False,
        }
        for name, before_call in expected.items():
            with self.subTest(command=name):
                self.assertEqual(self.hooks[name].before_call, before_call)

    def test_uploads_copies_and_clears_are_prepared_before_their_key(self) -> None:
        # Each publishes a pending invalidation of a persistent mapping before it writes the
        # buffer, whatever range it writes. Copies and clears also mark the range they write in
        # a mapping until they read it back.
        expected = {
            'glBufferSubData':
                'BeforeSubData(BufferRef::ByTarget(target), static_cast<uint64_t>(offset), '
                'static_cast<uint64_t>(size), data)',
            'glNamedBufferSubDataEXT':
                'BeforeSubData(BufferRef::ByName(buffer), static_cast<uint64_t>(offset), '
                'static_cast<uint64_t>(size), data)',
            'glCopyBufferSubData':
                'BeforeGpuWrite(BufferRef::ByTarget(writeTarget), '
                'static_cast<uint64_t>(writeOffset), static_cast<uint64_t>(size))',
            'glCopyNamedBufferSubData':
                'BeforeGpuWrite(BufferRef::ByName(writeBuffer), '
                'static_cast<uint64_t>(writeOffset), static_cast<uint64_t>(size))',
            'glNamedCopyBufferSubDataEXT':
                'BeforeGpuWrite(BufferRef::ByName(writeBuffer), '
                'static_cast<uint64_t>(writeOffset), static_cast<uint64_t>(size))',
            'glClearBufferSubData':
                'BeforeGpuWrite(BufferRef::ByTarget(target), static_cast<uint64_t>(offset), '
                'static_cast<uint64_t>(size))',
            'glClearBufferData': 'BeforeGpuWrite(BufferRef::ByTarget(target))',
            'glClearNamedBufferDataEXT': 'BeforeGpuWrite(BufferRef::ByName(buffer))',
            'glBufferData': None,
            'glTexBuffer': None,
            'glGetQueryBufferObjectiv': None,
        }
        for name, prepare in expected.items():
            with self.subTest(command=name):
                self.assertEqual(self.hooks[name].prepare, prepare)

    def test_readers_are_not_hooked(self) -> None:
        for name in ('glGetBufferSubData', 'glBindVertexBuffer', 'glVertexArrayElementBuffer',
                     'glUnmapBuffer', 'glGetBufferParameteriv'):
            with self.subTest(command=name):
                self.assertNotIn(name, self.hooks)

    def test_every_family_member_is_hooked(self) -> None:
        # A family member whose parameters do not match would silently go unhooked, letting a
        # buffer change without its recorder copy noticing.
        for cmd in self.commands:
            if any(pattern.match(cmd.name) for pattern, _ in _FAMILIES):
                with self.subTest(command=cmd.name):
                    self.assertIn(cmd.name, self.hooks)

    def test_disabled_commands_are_hooked(self) -> None:
        disabled = [cmd for cmd in self.commands if not cmd.enabled]
        self.assertTrue(buffer_content_hooks(disabled))


if __name__ == '__main__':
    unittest.main()
