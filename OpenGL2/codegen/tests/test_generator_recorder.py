# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Covers generator_recorder.py's PRE_HOOK_KEY_PAIR_COMMANDS table and the
# recorderWrappersAuto.cpp.mako shape it drives - a liveness-critical 1:1 contract with
# CaptureManager::SetPendingPreHookKey()/TakePendingPreHookKey() (see captureManager.h's own
# header comments): a command missing from this table but hooked by
# WindowGeometryTrackingLayer::Pre() would assert in TakePendingPreHookKey() (nothing ever set
# the key it tries to consume); a command wrongly ADDED to this table with no corresponding
# Pre() hook would reserve a key nothing ever consumes, permanently stalling
# OrderingRecorder for every higher-keyed command. Neither failure is a compile error, so this
# module tests both halves directly: the table's own exact membership, and the generated
# wrapper's actual reservation/hand-off shape for a command in (and out of) that table.

import re
import sys
import unittest
from pathlib import Path

CODEGEN_DIR = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(CODEGEN_DIR))

from generator_helpers import mako_render
from generator_recorder import (POST_HOOK_KEY_PAIR_COMMANDS, PRE_HOOK_KEY_PAIR_COMMANDS,
                                PRE_SWAP_TEXTURE_KEY_COMMANDS)
from intermediates import Api, CmdType, Command, ReturnValue
from intermediates_creator import (
    FILE_TO_API,
    extract_commands_from_registry,
    get_commands_from_xml_data,
)
from registry import Registry

TEMPLATE_PATH = str(CODEGEN_DIR / 'templates' / 'recorderWrappersAuto.cpp.mako')


def fake_command(name: str) -> Command:
    """Minimal disabled Command - the ordering-key reservation shape under test (both the
    single-key `else` branch and the PRE_HOOK_KEY_PAIR_COMMANDS branch) is emitted unconditionally,
    before this function's own `% if cmd.enabled:` check, so no params/return-value machinery is
    needed to exercise it."""
    return Command(
        name=name, api=Api.WGL, enabled=False, command_type=CmdType.NONE,
        return_value=ReturnValue(type='void'), params=[])


def render_wrappers(command_names: tuple[str, ...], pre_hook_key_pair_commands: frozenset[str],
                    post_hook_key_pair_commands: frozenset[str] = frozenset(),
                    pre_swap_texture_key_commands: frozenset[str] = frozenset()) -> str:
    return render_commands([fake_command(name) for name in command_names],
                           pre_hook_key_pair_commands, post_hook_key_pair_commands,
                           pre_swap_texture_key_commands)


def render_commands(commands: list[Command], pre_hook_key_pair_commands: frozenset[str],
                    post_hook_key_pair_commands: frozenset[str] = frozenset(),
                    pre_swap_texture_key_commands: frozenset[str] = frozenset()) -> str:
    rendered = mako_render(
        TEMPLATE_PATH, commands=commands, handle_creating_commands=frozenset(),
        handle_param_to_erase_on_success={}, mapped_handle_service_names={},
        context_share_param={}, share_lists_commands={},
        pre_hook_key_pair_commands=pre_hook_key_pair_commands,
        post_hook_key_pair_commands=post_hook_key_pair_commands,
        pre_swap_texture_key_commands=pre_swap_texture_key_commands, client_array_setters={},
        buffer_content_hooks={})
    assert rendered is not None, 'recorderWrappersAuto.cpp.mako failed to render - see stderr'
    return rendered


def extract_wrapper_body(rendered: str, name: str) -> str:
    """Isolates one Wrapper_<name> function's own source, so assertions about one command's
    reservation shape can never accidentally match a DIFFERENT command's own generated text."""
    match = re.search(
        rf'APIENTRY Wrapper_{re.escape(name)}\(.*?\n\}}\n', rendered, re.DOTALL)
    assert match is not None, f'Wrapper_{name} not found in rendered output'
    return match.group(0)


class PreHookKeyPairCommandsTableTests(unittest.TestCase):
    """Pins PRE_HOOK_KEY_PAIR_COMMANDS to exactly WindowGeometryTrackingLayer's four hooked WGL
    commands (see that layer's own header comment) - any other membership silently breaks the
    SetPendingPreHookKey()/TakePendingPreHookKey() 1:1 contract at runtime, not at compile time,
    so this table's exact contents must be pinned rather than merely smoke-tested."""

    def test_exact_membership(self) -> None:
        self.assertEqual(
            PRE_HOOK_KEY_PAIR_COMMANDS,
            frozenset({'wglSetPixelFormat', 'wglMakeCurrent', 'wglSwapBuffers',
                       'wglSwapLayerBuffers'}))


class PreHookKeyPairWrapperShapeTests(unittest.TestCase):
    """Renders the real recorderWrappersAuto.cpp.mako template (not a re-implementation of it)
    against PRE_HOOK_KEY_PAIR_COMMANDS's real, current membership, and checks the actual
    generated C++ shape for both branches."""

    @classmethod
    def setUpClass(cls) -> None:
        # 'wglCreateContext' is a real WGL command deliberately NOT in
        # PRE_HOOK_KEY_PAIR_COMMANDS - the control for the single-key `else` branch.
        command_names = (*sorted(PRE_HOOK_KEY_PAIR_COMMANDS), 'wglCreateContext')
        cls.rendered = render_wrappers(command_names, PRE_HOOK_KEY_PAIR_COMMANDS)

    def test_every_pre_hook_key_pair_command_reserves_a_pair_and_hands_off_the_spare_key(
            self) -> None:
        for name in sorted(PRE_HOOK_KEY_PAIR_COMMANDS):
            with self.subTest(command=name):
                body = extract_wrapper_body(self.rendered, name)
                self.assertIn('CaptureManager::Get().CreateCommandKeyPair()', body)
                self.assertIn(
                    'CaptureManager::Get().SetPendingPreHookKey(preHookKeyPair.first);', body)
                self.assertIn('const uint64_t orderingKey = preHookKeyPair.second;', body)
                # The single-key `else` branch must not ALSO fire for the same command - the
                # two reservation shapes are mutually exclusive per generator_recorder.py's own
                # `% if cmd.name in pre_hook_key_pair_commands: ... % else: ... % endif`.
                self.assertNotIn(
                    'const uint64_t orderingKey = record ? CaptureManager::Get()'
                    '.CreateCommandKey() : 0;', body)

    def test_a_command_outside_the_table_reserves_a_single_key_with_no_pending_hookup(
            self) -> None:
        body = extract_wrapper_body(self.rendered, 'wglCreateContext')
        self.assertIn(
            'const uint64_t orderingKey = record ? CaptureManager::Get().CreateCommandKey() : 0;',
            body)
        self.assertNotIn('CreateCommandKeyPair', body)
        self.assertNotIn('SetPendingPreHookKey', body)

    def test_table_membership_alone_drives_the_branch_choice(self) -> None:
        # Generic control, independent of PRE_HOOK_KEY_PAIR_COMMANDS's real, current contents
        # (see test_handle_lifecycle.py's own HypotheticalThirdHandleTypeTests for the same
        # rationale): a command named 'SoleHypotheticalCommand', reserving under a LOCAL,
        # single-entry table, must still take the pair branch - proving the template dispatches
        # on table membership alone, not on any of these four real names being hardcoded
        # anywhere in the template itself.
        rendered = render_wrappers(
            ('SoleHypotheticalCommand',), frozenset({'SoleHypotheticalCommand'}))
        body = extract_wrapper_body(rendered, 'SoleHypotheticalCommand')
        self.assertIn('CaptureManager::Get().CreateCommandKeyPair()', body)
        self.assertIn(
            'CaptureManager::Get().SetPendingPreHookKey(preHookKeyPair.first);', body)


class PostHookKeyPairTests(unittest.TestCase):
    def test_exact_membership(self) -> None:
        self.assertEqual(POST_HOOK_KEY_PAIR_COMMANDS, frozenset({'glGetUniformLocation'}))

    def test_wrapper_keeps_the_first_key_and_releases_an_unconsumed_second(self) -> None:
        rendered = render_wrappers(('glGetUniformLocation',), frozenset(),
                                   POST_HOOK_KEY_PAIR_COMMANDS)
        body = extract_wrapper_body(rendered, 'glGetUniformLocation')
        self.assertIn('CaptureManager::Get().CreateCommandKeyPair()', body)
        self.assertIn('const uint64_t orderingKey = postHookKeyPair.first;', body)
        self.assertIn('CaptureManager::Get().SetPendingPostHookKey(postHookKeyPair.second);', body)
        self.assertIn('CaptureManager::Get().ReleasePendingPostHookKey();', body)
        self.assertNotIn('SetPendingPreHookKey', body)


class PreSwapTextureKeyTests(unittest.TestCase):
    def test_exact_membership(self) -> None:
        self.assertEqual(PRE_SWAP_TEXTURE_KEY_COMMANDS,
                         frozenset({'wglSwapBuffers', 'wglSwapLayerBuffers'}))

    def test_texture_key_precedes_the_pre_hook_pair_in_one_reservation(self) -> None:
        rendered = render_wrappers(('wglSwapBuffers', 'wglMakeCurrent'),
                                   PRE_HOOK_KEY_PAIR_COMMANDS,
                                   pre_swap_texture_key_commands=PRE_SWAP_TEXTURE_KEY_COMMANDS)
        body = extract_wrapper_body(rendered, 'wglSwapBuffers')
        self.assertIn('CaptureManager::Get().CreateCommandKeyRange(3)', body)
        self.assertIn('CaptureManager::Get().SetPendingMappedTextureKey(firstKey);', body)
        self.assertIn('{firstKey + 1, firstKey + 2}', body)
        self.assertIn('SetPendingPreHookKey(preHookKeyPair.first);', body)
        self.assertIn('const uint64_t orderingKey = preHookKeyPair.second;', body)
        self.assertNotIn('CaptureManager::Get().CreateCommandKeyPair()', body)

        other = extract_wrapper_body(rendered, 'wglMakeCurrent')
        self.assertIn('CaptureManager::Get().CreateCommandKeyPair()', other)
        self.assertNotIn('SetPendingMappedTextureKey', other)


def registry_command(name: str) -> Command:
    path = next(path for path, api in FILE_TO_API.items() if api == Api.GL)
    registry = Registry()
    registry.loadFile(path)
    raw = next(c for c in extract_commands_from_registry(registry)
               if c['command_attribs']['name'] == name)
    return get_commands_from_xml_data({Api.GL: [raw]})[Api.GL][0]


class ExistenceQueryWrapperTests(unittest.TestCase):
    """glIs* keeps a registered name's key, looked up without the unmapped fallback before the
    call, and confirms an unregistered name against the driver's result after it. Neither half
    missing nor emitted in the wrong phase is a compile error."""

    def test_lookup_precedes_the_call_and_confirmation_follows_it(self) -> None:
        body = extract_wrapper_body(
            render_commands([registry_command('glIsTexture')], frozenset()), 'glIsTexture')
        lookup = body.find(
            'UpdateHandleIfRegistered(command->m_texture, GLObjectKind::Texture);')
        call = body.find('GLboolean result = realFunction(texture);')
        confirm = body.find(
            'ConfirmExistenceQuery(command->m_texture, GLObjectKind::Texture, result);')
        self.assertNotEqual(lookup, -1, body)
        self.assertNotEqual(call, -1, body)
        self.assertNotEqual(confirm, -1, body)
        self.assertLess(lookup, call)
        self.assertLess(call, confirm)
        self.assertNotIn('UpdateHandle(command->m_texture', body)


if __name__ == '__main__':
    unittest.main()
