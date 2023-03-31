#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import copy
import unittest

import cl_utils


FUNCTIONS = {
    'test1': {
        'name': 'test1',
        'enabled': True,
        'type': 1,
        'availableFrom': '1.0',
        'retV': {
            'type': 'cl_int',
            'wrapType': 'CCLResult',
            'wrapParams': 'test'
        },
        'arg1': {
            'name': 'param1',
            'type': 'cl_program'
        },
        'arg2': {
            'name': 'param2',
            'type': 'cl_uint'
        }
    },
    'test2': {
        'name': 'test2',
        'enabled': False,
        'availableFrom': '1.2',
        'type': 2,
        'inheritFrom': 'test1'
    },
    'test3': {
        'name': 'test3',
        'enabled': True,
        'availableFrom': '1.0',
        'type': 3,
        'retV': {
            'type': 'cl_uint'
        }
    }
}


class ProcessTestCase(unittest.TestCase):
    """Tests for function processor."""
    @classmethod
    def setUpClass(cls):
        cls.PROCESSED = copy.deepcopy(FUNCTIONS)
        cl_utils.process(cls.PROCESSED)

    def setUp(self):
        self.test1 = self.PROCESSED['test1']
        self.test2 = self.PROCESSED['test2']
        self.test3 = self.PROCESSED['test3']
        self.test1_orig = FUNCTIONS['test1']
        self.test2_orig = FUNCTIONS['test2']
        self.test3_orig = FUNCTIONS['test3']

    def test_implicit_properties(self):
        self.assertIn('recWrapName', self.test1)
        self.assertIn('recExecWrapName', self.test1)
        self.assertIn('runWrapName', self.test1)
        self.assertIn('stateTrackName', self.test1)

    def test_return_type(self):
        self.assertEqual(self.test3['type'], self.test3['wrapType'])

    def test_wrap_return_type(self):
        self.assertEqual(self.test1['type'], self.test1_orig['retV']['type'])
        self.assertEqual(self.test1['wrapType'], self.test1_orig['retV']['wrapType'])
        self.assertEqual(self.test1['wrapParams'], self.test1_orig['retV']['wrapParams'])

    def test_arguments(self):
        self.assertIn('args', self.test1)
        self.assertEqual(self.test1['args'][0]['name'], self.test1_orig['arg1']['name'])
        self.assertEqual(self.test1['args'][0]['type'], self.test1_orig['arg1']['type'])
        self.assertEqual(self.test1['args'][1]['wrapType'], self.test1['args'][1]['type'])

    def test_inheritance_name(self):
        self.assertEqual(self.test1['name'], self.test1_orig['name'])
        self.assertEqual(self.test2['name'], self.test2_orig['name'])

    def test_inheritance_properties(self):
        self.assertEqual(self.test2['enabled'], self.test2_orig['enabled'])
        self.assertEqual(self.test2['availableFrom'], self.test2_orig['availableFrom'])
        self.assertEqual(self.test2['functionType'], self.test1['functionType'])
        self.assertEqual(self.test2['type'], self.test1['type'])

    def test_inheritance_implicit_properties(self):
        self.assertEqual(self.test2['recWrapName'], self.test2['name'])
        self.assertEqual(self.test2['recExecWrapName'], self.test1['recExecWrapName'])
        self.assertEqual(self.test2['runWrapName'], self.test1['runWrapName'])
        self.assertEqual(self.test2['stateTrackName'], self.test1['stateTrackName'])

    # TODO: test_wrap_return_type
    # TODO: test_inheritance_return_type
    # TODO: test_inheritance_arguments

if __name__ == '__main__':
    unittest.main()
