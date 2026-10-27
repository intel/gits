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
from generator_recorder import (POST_HOOK_KEY_PAIR_COMMANDS, PRE_HOOK_EXTRA_KEY_COMMANDS,
                                PRE_HOOK_KEY_PAIR_COMMANDS, DrawableIdentities,
                                drawable_identities)
from handle_lifecycle import FRAME_END_COMMANDS
from intermediates import Api, CmdType, Command, ReturnValue
from intermediates_creator import (
    FILE_TO_API,
    extract_commands_from_registry,
    get_commands_from_xml_data,
)
from registry import Registry

TEMPLATE_PATH = str(CODEGEN_DIR / 'templates' / 'recorderWrappersAuto.cpp.mako')


def fake_command(name: str, enabled: bool = False) -> Command:
    """Minimal disabled Command - the ordering-key reservation shape under test (both the
    single-key `else` branch and the PRE_HOOK_KEY_PAIR_COMMANDS branch) is emitted unconditionally,
    before this function's own `% if cmd.enabled:` check, so no params/return-value machinery is
    needed to exercise it."""
    return Command(
        name=name, api=Api.WGL, enabled=enabled, command_type=CmdType.NONE,
        return_value=ReturnValue(type='void'), params=[])


def render_wrappers(command_names: tuple[str, ...], pre_hook_key_pair_commands: frozenset[str],
                    post_hook_key_pair_commands: frozenset[str] = frozenset(),
                    pre_hook_extra_key_commands: dict[str, str] | None = None) -> str:
    return render_commands([fake_command(name) for name in command_names],
                           pre_hook_key_pair_commands, post_hook_key_pair_commands,
                           pre_hook_extra_key_commands)


def render_commands(commands: list[Command], pre_hook_key_pair_commands: frozenset[str],
                    post_hook_key_pair_commands: frozenset[str] = frozenset(),
                    pre_hook_extra_key_commands: dict[str, str] | None = None,
                    frame_end_commands: frozenset[str] = frozenset()) -> str:
    rendered = mako_render(
        TEMPLATE_PATH, commands=commands, handle_creating_commands=frozenset(),
        handle_param_to_erase_on_success={}, mapped_handle_service_names={},
        context_share_param={}, share_lists_commands={},
        pre_hook_key_pair_commands=pre_hook_key_pair_commands,
        post_hook_key_pair_commands=post_hook_key_pair_commands,
        pre_hook_extra_key_commands=pre_hook_extra_key_commands or {},
        frame_end_commands=frame_end_commands, client_array_setters={},
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
    """Pins PRE_HOOK_KEY_PAIR_COMMANDS to exactly WindowGeometryTrackingLayer's hooked WGL and
    GLX commands (see that layer's own header comment) plus PixelFormatCaptureLayer's
    wglCreatePbufferARB/EXT and GLX commands taking a GLXFBConfig or a visual - any other
    membership silently breaks the SetPendingPreHookKey()/TakePendingPreHookKey() 1:1 contract
    at runtime, not at compile time, so this table's exact contents must be pinned rather than
    merely smoke-tested."""

    def test_exact_membership(self) -> None:
        self.assertEqual(
            PRE_HOOK_KEY_PAIR_COMMANDS,
            frozenset({'wglSetPixelFormat', 'wglCreateContext', 'wglCreateContextAttribsARB',
                       'wglMakeCurrent', 'wglSwapBuffers', 'wglSwapLayerBuffers',
                       'wglCreatePbufferARB', 'wglCreatePbufferEXT',
                       'wglMakeContextCurrentARB', 'wglMakeContextCurrentEXT',
                       'glXMakeCurrent', 'glXMakeContextCurrent', 'glXMakeCurrentReadSGI',
                       'glXSwapBuffers', 'glXGetVisualFromFBConfig', 'glXCreateWindow',
                       'glXCreatePbuffer', 'glXCreateContext', 'glXCreateNewContext',
                       'glXCreateContextAttribsARB'}))


class PreHookKeyPairWrapperShapeTests(unittest.TestCase):
    """Renders the real recorderWrappersAuto.cpp.mako template (not a re-implementation of it)
    against PRE_HOOK_KEY_PAIR_COMMANDS's real, current membership, and checks the actual
    generated C++ shape for both branches."""

    @classmethod
    def setUpClass(cls) -> None:
        # 'wglDeleteContext' is a real WGL command deliberately NOT in
        # PRE_HOOK_KEY_PAIR_COMMANDS - the control for the single-key `else` branch.
        command_names = (*sorted(PRE_HOOK_KEY_PAIR_COMMANDS), 'wglDeleteContext')
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
        body = extract_wrapper_body(self.rendered, 'wglDeleteContext')
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
        # on table membership alone, not on any of these real names being hardcoded
        # anywhere in the template itself.
        rendered = render_wrappers(
            ('SoleHypotheticalCommand',), frozenset({'SoleHypotheticalCommand'}))
        body = extract_wrapper_body(rendered, 'SoleHypotheticalCommand')
        self.assertIn('CaptureManager::Get().CreateCommandKeyPair()', body)
        self.assertIn(
            'CaptureManager::Get().SetPendingPreHookKey(preHookKeyPair.first);', body)


class PostHookKeyPairTests(unittest.TestCase):
    def test_exact_membership(self) -> None:
        self.assertEqual(POST_HOOK_KEY_PAIR_COMMANDS,
                         frozenset({'glGetUniformLocation', 'glGetUniformLocationARB'}))

    def test_wrapper_keeps_the_first_key_and_releases_an_unconsumed_second(self) -> None:
        rendered = render_wrappers(('glGetUniformLocation',), frozenset(),
                                   POST_HOOK_KEY_PAIR_COMMANDS)
        body = extract_wrapper_body(rendered, 'glGetUniformLocation')
        self.assertIn('CaptureManager::Get().CreateCommandKeyPair()', body)
        self.assertIn('const uint64_t orderingKey = postHookKeyPair.first;', body)
        self.assertIn('CaptureManager::Get().SetPendingPostHookKey(postHookKeyPair.second);', body)
        self.assertIn('CaptureManager::Get().ReleasePendingPostHookKey();', body)
        self.assertNotIn('SetPendingPreHookKey', body)


class PreHookExtraKeyTests(unittest.TestCase):
    def test_exact_membership(self) -> None:
        self.assertEqual(PRE_HOOK_EXTRA_KEY_COMMANDS,
                         {'wglSwapBuffers': 'SetPendingMappedTextureKey',
                          'wglSwapLayerBuffers': 'SetPendingMappedTextureKey',
                          'glXSwapBuffers': 'SetPendingMappedTextureKey',
                          'wglSetPixelFormat': 'SetPendingPixelFormatKey',
                          'glXCreateWindow': 'SetPendingPixelFormatKey'})

    def test_extra_key_precedes_the_pre_hook_pair_in_one_reservation(self) -> None:
        rendered = render_wrappers(('wglSwapBuffers', 'wglSetPixelFormat', 'wglMakeCurrent'),
                                   PRE_HOOK_KEY_PAIR_COMMANDS,
                                   pre_hook_extra_key_commands=PRE_HOOK_EXTRA_KEY_COMMANDS)
        for name, setter in (('wglSwapBuffers', 'SetPendingMappedTextureKey'),
                             ('wglSetPixelFormat', 'SetPendingPixelFormatKey')):
            with self.subTest(command=name):
                body = extract_wrapper_body(rendered, name)
                self.assertIn('CaptureManager::Get().CreateCommandKeyRange(3)', body)
                self.assertIn(f'CaptureManager::Get().{setter}(firstKey);', body)
                self.assertIn('{firstKey + 1, firstKey + 2}', body)
                self.assertIn('SetPendingPreHookKey(preHookKeyPair.first);', body)
                self.assertIn('const uint64_t orderingKey = preHookKeyPair.second;', body)
                self.assertNotIn('CaptureManager::Get().CreateCommandKeyPair()', body)

        other = extract_wrapper_body(rendered, 'wglMakeCurrent')
        self.assertIn('CaptureManager::Get().CreateCommandKeyPair()', other)
        self.assertNotIn('firstKey', other)

    def test_frame_end_is_pending_from_its_reservation_until_its_pre_layers_ran(self) -> None:
        # RecorderScreenshotsLayer numbers swaps in key order, so a swap with a lower key must be
        # known from the moment its key exists, not only once its Pre layers see it.
        rendered = render_commands([fake_command('wglSwapBuffers', enabled=True)],
                                   PRE_HOOK_KEY_PAIR_COMMANDS,
                                   pre_hook_extra_key_commands=PRE_HOOK_EXTRA_KEY_COMMANDS,
                                   frame_end_commands=FRAME_END_COMMANDS)
        body = extract_wrapper_body(rendered, 'wglSwapBuffers')
        self.assertIn('CaptureManager::Get().CreateFrameEndKeyRange(3)', body)
        self.assertNotIn('CreateCommandKeyRange', body)
        pre_layers = body.find('preLayer->Pre(*command);')
        ran = body.find('CaptureManager::Get().FrameEndPreLayersRan(orderingKey);')
        call = body.find('realFunction(')
        self.assertNotEqual(pre_layers, -1, body)
        self.assertLess(pre_layers, ran)
        self.assertLess(ran, call)

def registry_command(name: str, api: Api = Api.GL) -> Command:
    path = next(path for path, file_api in FILE_TO_API.items() if file_api == api)
    registry = Registry()
    registry.loadFile(path)
    raw = next(c for c in extract_commands_from_registry(registry)
               if c['command_attribs']['name'] == name)
    return get_commands_from_xml_data({api: [raw]})[api][0]


class ExistenceQueryWrapperTests(unittest.TestCase):
    """glIs* keeps a registered name's key, looked up without the unmapped fallback before the
    call, and confirms an unregistered name against the driver's result after it. Neither half
    missing nor emitted in the wrong phase is a compile error."""

    def test_lookup_precedes_the_call_and_confirmation_follows_it(self) -> None:
        body = extract_wrapper_body(
            render_commands([registry_command('glIsTexture')], frozenset()), 'glIsTexture')
        lookup = body.find(
            'UpdateHandleIfRegistered(command->m_texture, GLObjectKind::Texture);')
        call = body.find('realFunction(texture);')
        confirm = body.find(
            'ConfirmExistenceQuery(command->m_texture, GLObjectKind::Texture, result);')
        self.assertNotEqual(lookup, -1, body)
        self.assertNotEqual(call, -1, body)
        self.assertNotEqual(confirm, -1, body)
        self.assertLess(lookup, call)
        self.assertLess(call, confirm)
        self.assertNotIn('UpdateHandle(command->m_texture', body)


class LayerReplacedCallWrapperTests(unittest.TestCase):
    """A Pre layer that sets m_Skip answers the command itself: through the application's OUT
    pointers, which it must have before the Pre layers run, or through m_Return, which the
    application gets back after the Post layers, so they can replace it too."""

    @classmethod
    def setUpClass(cls) -> None:
        cls.rendered = render_commands(
            [registry_command(name) for name in ('glGetIntegerv', 'glGetString', 'glIsTexture')],
            frozenset())

    def test_out_pointer_is_set_before_the_pre_layers(self) -> None:
        body = extract_wrapper_body(self.rendered, 'glGetIntegerv')
        pointer = body.find('command->m_data.Pointer = static_cast<GLint*>(data);')
        pre = body.find('preLayer->Pre(*command);')
        self.assertNotEqual(pointer, -1, body)
        self.assertLess(pointer, pre)

    def test_void_call_is_skipped(self) -> None:
        body = extract_wrapper_body(self.rendered, 'glGetIntegerv')
        self.assertIn('if (!record || !command->m_Skip) {\n    realFunction(pname, data);', body)

    def test_skipped_call_returns_m_return(self) -> None:
        body = extract_wrapper_body(self.rendered, 'glIsTexture')
        self.assertIn('GLboolean result = record && command->m_Skip ? command->m_Return.Value : '
                      'realFunction(texture);', body)
        self.assertIn('return record ? command->m_Return.Value : result;', body)

    def test_string_result_reaches_the_post_layers(self) -> None:
        body = extract_wrapper_body(self.rendered, 'glGetString')
        assign = body.find('command->m_Return.Value = result;')
        post = body.find('postLayer->Post(*command);')
        self.assertNotEqual(assign, -1, body)
        self.assertLess(assign, post)
        self.assertIn('return record ? command->m_Return.Value : result;', body)


class EncoderLayerSkipRecordingTests(unittest.TestCase):
    """A command a recorder layer leaves out of the stream must still release its key, or every
    later command stalls in OrderingRecorder."""

    def test_skip_recording_releases_the_key(self) -> None:
        rendered = mako_render(
            str(CODEGEN_DIR / 'templates' / 'encoderLayerAuto.cpp.mako'),
            commands=[registry_command('glProgramBinary')], frame_end_commands=frozenset(),
            drawable_identities={})
        self.assertIsNotNone(rendered)
        self.assertIn('  if (command.m_SkipRecording) {\n'
                      '    CaptureManager::Get().SkipCommandKey(command.m_Key);\n'
                      '    return;\n'
                      '  }\n'
                      '  CaptureManager::Get().RecordCommand(', rendered)


class DrawableIdentityTests(unittest.TestCase):
    """Two X servers can give different drawables the same XID, so every drawable a GLX command
    names is recorded with its server, and a drawable recorded as a bare XID would be replayed as
    a drawable of the first server."""

    @classmethod
    def setUpClass(cls) -> None:
        cls.commands = [registry_command(name, Api.GLX) for name in (
            'glXSwapBuffers', 'glXMakeContextCurrent', 'glXCreateWindow',
            'glXGetCurrentDrawable', 'glXCreateContext')]
        for command in cls.commands:
            command.enabled = True
        cls.identities = drawable_identities(cls.commands)

    def test_drawable_params_and_returns_are_identities_on_the_command_display(self) -> None:
        self.assertEqual(self.identities['glXMakeContextCurrent'],
                         DrawableIdentities('command.m_dpy.Value', ('draw', 'read'), False))
        self.assertEqual(self.identities['glXCreateWindow'],
                         DrawableIdentities('command.m_dpy.Value', ('win',), True))

    def test_a_command_without_a_display_uses_the_current_one(self) -> None:
        self.assertEqual(
            self.identities['glXGetCurrentDrawable'],
            DrawableIdentities(
                'g_DispatchTable.glXGetCurrentDisplay.load(std::memory_order_acquire)()', (), True))

    def test_commands_naming_no_drawable_are_recorded_unchanged(self) -> None:
        self.assertNotIn('glXCreateContext', self.identities)

    def test_disabled_commands_are_ignored(self) -> None:
        command = registry_command('glXSwapBuffers', Api.GLX)
        command.enabled = False
        self.assertEqual(drawable_identities([command]), {})

    def test_the_encoder_records_a_copy_with_identities(self) -> None:
        rendered = mako_render(
            str(CODEGEN_DIR / 'templates' / 'encoderLayerAuto.cpp.mako'),
            commands=self.commands, frame_end_commands=frozenset(),
            drawable_identities=self.identities)
        self.assertIsNotNone(rendered)
        self.assertIn(
            '  glXSwapBuffersCommand recorded = command;\n'
            '  Display* display = command.m_dpy.Value;\n'
            '  recorded.m_drawable.Value =\n'
            '      GlxDrawableIdentityService::Get().Identity(display, command.m_drawable.Value);\n'
            '  CaptureManager::Get().RecordCommand(recorded, '
            'new glXSwapBuffersCommandSerializer(recorded));\n', rendered)
        self.assertIn('  CaptureManager::Get().RecordCommand(command, '
                      'new glXCreateContextCommandSerializer(command));\n', rendered)


if __name__ == '__main__':
    unittest.main()
