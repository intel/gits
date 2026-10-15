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

from texture_sampling_commands import samples_textures, sampling_hook_after, sampling_hook_before


class TextureSamplingCommandsTests(unittest.TestCase):
    def test_sampling_commands(self) -> None:
        for name in (
            'glDrawArrays', 'glDrawElementsInstancedBaseVertexBaseInstance',
            'glMultiDrawElementsIndirect', 'glDrawArraysIndirect', 'glDrawTransformFeedback',
            'glDrawMeshTasksNV', 'glDrawPixels', 'glDispatchCompute',
            'glDispatchComputeIndirect', 'glRectf', 'glRectiv', 'glBegin', 'glCallList',
            'glCallLists', 'glBitmap', 'glCopyPixels', 'glEvalMesh2',
        ):
            with self.subTest(command=name):
                self.assertTrue(samples_textures(name))

    def test_non_sampling_commands(self) -> None:
        for name in (
            'glDrawBuffer', 'glDrawBuffers', 'glDrawBuffersARB', 'glDrawBuffersIndexedEXT',
            'glBeginQuery', 'glBeginTransformFeedback', 'glEnd', 'glBindTexture',
            'glTexParameteri', 'glMultiTexCoord2f', 'glRectangle',
        ):
            with self.subTest(command=name):
                self.assertFalse(samples_textures(name))

    def test_hook_before(self) -> None:
        self.assertEqual(sampling_hook_before('glDrawArrays'), 'BeforeSampling()')
        self.assertEqual(sampling_hook_before('glCallList'), 'BeforeCallList(command.m_list.Value)')
        self.assertTrue(sampling_hook_before('glCallLists').startswith('BeforeCallLists('))
        self.assertIsNone(sampling_hook_before('glBindTexture'))

    def test_hook_after(self) -> None:
        self.assertEqual(sampling_hook_after('glDrawArrays'), 'AfterSampling()')
        self.assertEqual(sampling_hook_after('glCallList'), 'AfterSampling()')
        self.assertEqual(sampling_hook_after('glBegin'), 'EnterBeginEnd()')
        self.assertEqual(sampling_hook_after('glEnd'), 'LeaveBeginEnd()')
        self.assertEqual(sampling_hook_after('glNewList'), 'BeginListCompile(command.m_list.Value)')
        self.assertEqual(sampling_hook_after('glEndList'), 'EndListCompile()')
        self.assertIsNone(sampling_hook_after('glBindTexture'))


if __name__ == '__main__':
    unittest.main()
