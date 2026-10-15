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
                'OnData(BufferRef::ByTarget(target), static_cast<uint64_t>(size), data)',
            'glNamedBufferStorage':
                'OnData(BufferRef::ByName(buffer), static_cast<uint64_t>(size), data)',
            'glBufferSubDataARB':
                'OnSubData(BufferRef::ByTarget(target), static_cast<uint64_t>(offset), '
                'static_cast<uint64_t>(size), data)',
            'glCopyBufferSubData': 'OnGpuWrite(BufferRef::ByTarget(writeTarget))',
            'glNamedCopyBufferSubDataEXT': 'OnGpuWrite(BufferRef::ByName(writeBuffer))',
            'glClearBufferSubData': 'OnGpuWrite(BufferRef::ByTarget(target))',
            'glTexBuffer': 'OnGpuWrite(BufferRef::ByName(buffer))',
            'glTransformFeedbackBufferRange': 'OnGpuWrite(BufferRef::ByName(buffer))',
            'glGetQueryBufferObjectuiv': 'OnGpuWrite(BufferRef::ByName(buffer))',
            'glBufferStorageMemEXT': 'OnGpuWrite(BufferRef::ByTarget(target))',
            'glInvalidateBufferSubData': 'OnUndefined(BufferRef::ByName(buffer))',
            'glMapNamedBufferRange': 'OnUndefined(BufferRef::ByName(buffer))',
            'glBindBufferARB': 'OnBind(target, buffer)',
            'glBindBufferRangeNV': 'OnBindIndexed(target, buffer)',
            'glBindBuffersBase': 'OnBindIndexed(target, count, buffers)',
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
            'glCopyBufferSubData': None,
        }
        for name, guard in expected.items():
            with self.subTest(command=name):
                self.assertEqual(self.hooks[name].guard, guard)

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
