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

from buffer_content_hooks import BufferContentHook
from generator_helpers import mako_render, mako_render_parts
from generator_recorder import (POST_HOOK_KEY_PAIR_COMMANDS, PRE_HOOK_EXTRA_KEY_COMMANDS,
                                PRE_HOOK_KEY_PAIR_COMMANDS, PRIMITIVE_PAIR_COMMANDS,
                                RECORDER_WRAPPER_PARTS,
                                DrawableIdentities, drawable_identities, mapped_write_publishers,
                                query_buffer_publishers)
from handle_lifecycle import (CONTEXT_SHARE_PARAM, FRAME_END_COMMANDS, HANDLE_CREATING_COMMANDS,
                              HANDLE_PARAM_TO_ERASE_ON_SUCCESS, MAPPED_HANDLE_SERVICE_NAMES,
                              SHARE_LISTS_COMMANDS)
from intermediates import Api, CmdType, Command, Parameter, ReturnValue
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
                    frame_end_commands: frozenset[str] = frozenset(),
                    mapped_write_publishers: frozenset[str] = frozenset(),
                    primitive_pair_commands: dict[str, bool] | None = None,
                    query_buffer_publishers: frozenset[str] = frozenset(),
                    buffer_content_hooks: dict[str, BufferContentHook] | None = None,
                    with_handle_lifecycles: bool = False) -> str:
    rendered = mako_render(
        TEMPLATE_PATH, commands=commands,
        **template_arguments(pre_hook_key_pair_commands, post_hook_key_pair_commands,
                             pre_hook_extra_key_commands, frame_end_commands,
                             mapped_write_publishers, primitive_pair_commands,
                             query_buffer_publishers, buffer_content_hooks,
                             with_handle_lifecycles))
    assert rendered is not None, 'recorderWrappersAuto.cpp.mako failed to render - see stderr'
    return rendered


def template_arguments(pre_hook_key_pair_commands: frozenset[str] = frozenset(),
                       post_hook_key_pair_commands: frozenset[str] = frozenset(),
                       pre_hook_extra_key_commands: dict[str, str] | None = None,
                       frame_end_commands: frozenset[str] = frozenset(),
                       mapped_write_publishers: frozenset[str] = frozenset(),
                       primitive_pair_commands: dict[str, bool] | None = None,
                       query_buffer_publishers: frozenset[str] = frozenset(),
                       buffer_content_hooks: dict[str, BufferContentHook] | None = None,
                       with_handle_lifecycles: bool = False) -> dict:
    """The template arguments other than commands and the part arguments."""
    return dict(
        handle_creating_commands=HANDLE_CREATING_COMMANDS if with_handle_lifecycles else frozenset(),
        handle_param_to_erase_on_success=(HANDLE_PARAM_TO_ERASE_ON_SUCCESS
                                          if with_handle_lifecycles else {}),
        mapped_handle_service_names=MAPPED_HANDLE_SERVICE_NAMES if with_handle_lifecycles else {},
        context_share_param=CONTEXT_SHARE_PARAM if with_handle_lifecycles else {},
        share_lists_commands=SHARE_LISTS_COMMANDS if with_handle_lifecycles else {},
        pre_hook_key_pair_commands=pre_hook_key_pair_commands,
        post_hook_key_pair_commands=post_hook_key_pair_commands,
        pre_hook_extra_key_commands=pre_hook_extra_key_commands or {},
        frame_end_commands=frame_end_commands, mapped_write_publishers=mapped_write_publishers,
        query_buffer_publishers=query_buffer_publishers,
        primitive_pair_commands=primitive_pair_commands or {},
        client_array_setters={}, buffer_content_hooks=buffer_content_hooks or {})


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
                       'glXCreateContextAttribsARB', 'glXCreatePixmap', 'glXCreateGLXPixmap'}))


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
                    'const uint64_t orderingKey = CaptureManager::Get().CreateCommandKey();', body)

    def test_a_command_outside_the_table_reserves_a_single_key_with_no_pending_hookup(
            self) -> None:
        body = extract_wrapper_body(self.rendered, 'wglDeleteContext')
        self.assertIn(
            'const uint64_t orderingKey = CaptureManager::Get().CreateCommandKey();', body)
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
                          'glXCreateWindow': 'SetPendingPixelFormatKey',
                          'glXCreatePixmap': 'SetPendingPixelFormatKey',
                          'glXCreateGLXPixmap': 'SetPendingPixelFormatKey'})

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
        pre_layers = body.find('preLayer->Pre(command);')
        ran = body.find('CaptureManager::Get().FrameEndPreLayersRan(orderingKey);')
        call = body.rfind('realFunction(')
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


def wrapper_paths(body: str) -> dict[str, str]:
    """The plugin path, taken while recording is disabled, and the recording path of an enabled
    command's wrapper."""
    plugin_start = body.find('if (!guard.IsNested() && PluginLayersLoaded()) {')
    recording_start = body.find('CaptureManager::Get().CreateCommandKey')
    return {'plugin': body[plugin_start:recording_start], 'recording': body[recording_start:]}


class ExistenceQueryWrapperTests(unittest.TestCase):
    """glIs* keeps a registered name's key, looked up without the unmapped fallback before the
    call, and confirms an unregistered name against the driver's result after it. Neither half
    missing nor emitted in the wrong phase is a compile error."""

    def test_lookup_precedes_the_call_and_confirmation_follows_it(self) -> None:
        body = extract_wrapper_body(
            render_commands([registry_command('glIsTexture')], frozenset()), 'glIsTexture')
        for name, path in wrapper_paths(body).items():
            with self.subTest(path=name):
                lookup = path.find(
                    'UpdateHandleIfRegistered(command.m_texture, GLObjectKind::Texture);')
                call = path.find('realFunction(texture);')
                confirm = path.find(
                    'ConfirmExistenceQuery(command.m_texture, GLObjectKind::Texture, result);')
                self.assertNotEqual(lookup, -1, path)
                self.assertNotEqual(call, -1, path)
                self.assertNotEqual(confirm, -1, path)
                self.assertLess(lookup, call)
                self.assertLess(call, confirm)
        self.assertNotIn('UpdateHandle(command.m_texture', body)


class PluginPathPointerTests(unittest.TestCase):
    """While recording is disabled, plugin layers see a buffer offset passed for an IN pointer,
    which the argument guards hide from the pointer's array."""

    def test_buffer_offset_of_indices_reaches_the_pre_layers(self) -> None:
        body = extract_wrapper_body(
            render_commands([registry_command('glDrawElements')], frozenset()), 'glDrawElements')
        path = wrapper_paths(body)['plugin']
        raw = path.find(
            'command.m_indicesRawPointer.Value = reinterpret_cast<uintptr_t>(indices);')
        self.assertNotEqual(raw, -1, path)
        self.assertLess(raw, path.find('preLayer->Pre(command);'))


class PluginPathIdentityTests(unittest.TestCase):
    """While recording is disabled, plugin layers identify GL objects, contexts and their share
    groups by the same keys as while recording."""

    @classmethod
    def setUpClass(cls) -> None:
        commands = [registry_command(name) for name in
                    ('glGenTextures', 'glDeleteTextures', 'glUseProgram')]
        commands += [registry_command(name, Api.WGL) for name in
                     ('wglCreateContext', 'wglDeleteContext', 'wglShareLists')]
        rendered = render_commands(commands, frozenset(), with_handle_lifecycles=True)
        cls.paths = {name: wrapper_paths(extract_wrapper_body(rendered, name))['plugin']
                     for name in (command.name for command in commands)}

    def assert_between_call_and_post_layers(self, name: str, text: str) -> None:
        path = self.paths[name]
        position = path.find(text)
        self.assertNotEqual(position, -1, path)
        self.assertLess(path.find('realFunction('), position)
        self.assertLess(position, path.find('postLayer->Post(command);'))

    def test_bound_program_key_is_looked_up_before_the_pre_layers(self) -> None:
        path = self.paths['glUseProgram']
        lookup = path.find('UpdateHandle(command.m_program, GLObjectKind::Program);')
        self.assertNotEqual(lookup, -1, path)
        self.assertLess(lookup, path.find('preLayer->Pre(command);'))

    def test_generated_names_are_read_and_registered(self) -> None:
        path = self.paths['glGenTextures']
        capture = path.find('command.m_textures.Capture(textures, n);')
        self.assert_between_call_and_post_layers(
            'glGenTextures', 'UpdateOutputHandleArray(command.m_textures, GLObjectKind::Texture);')
        self.assertLess(path.find('realFunction('), capture)
        self.assertLess(capture, path.find('UpdateOutputHandleArray('))

    def test_deleted_names_are_forgotten(self) -> None:
        self.assert_between_call_and_post_layers(
            'glDeleteTextures', 'ObjectNameMapService::Get().Delete(GLObjectKind::Texture,')

    def test_context_is_registered_with_its_share_group(self) -> None:
        self.assert_between_call_and_post_layers('wglCreateContext',
                                                 'UpdateOutputHandle(command.m_Return);')
        self.assert_between_call_and_post_layers('wglCreateContext',
                                                 'ShareGroupService::Get().RegisterNewContext(')

    def test_deleted_context_is_erased_on_success(self) -> None:
        self.assert_between_call_and_post_layers(
            'wglDeleteContext', 'if (result) {\n        ContextHandleMapService::Get().Erase(')

    def test_shared_lists_merge_share_groups(self) -> None:
        self.assert_between_call_and_post_layers('wglShareLists',
                                                 'ShareGroupService::Get().MergeGroups(')


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
        # Both the recording path and the plugin path, taken while recording is disabled.
        body = extract_wrapper_body(self.rendered, 'glGetIntegerv')
        pointers = [m.start() for m in
                    re.finditer(re.escape('command.m_data.Pointer = static_cast<GLint*>(data);'),
                                body)]
        pres = [m.start() for m in re.finditer(re.escape('preLayer->Pre(command);'), body)]
        self.assertEqual(len(pointers), 2, body)
        self.assertEqual(len(pres), 2, body)
        for pointer, pre in zip(pointers, pres):
            self.assertLess(pointer, pre)

    def test_void_call_is_skipped(self) -> None:
        body = extract_wrapper_body(self.rendered, 'glGetIntegerv')
        self.assertIn('if (!command.m_Skip) {\n    realFunction(pname, data);', body)

    def test_skipped_call_returns_m_return(self) -> None:
        body = extract_wrapper_body(self.rendered, 'glIsTexture')
        self.assertIn('GLboolean result = command.m_Skip ? command.m_Return.Value : '
                      'realFunction(texture);', body)
        self.assertIn('return command.m_Return.Value;', body)

    def test_string_result_reaches_the_post_layers(self) -> None:
        body = extract_wrapper_body(self.rendered, 'glGetString')
        assign = body.find('command.m_Return.Value = result;')
        post = body.find('postLayer->Post(command);')
        self.assertNotEqual(assign, -1, body)
        self.assertLess(assign, post)
        self.assertIn('return command.m_Return.Value;', body)


class NestedCallWrapperTests(unittest.TestCase):
    """A call nested in another wrapper's call only reaches the driver, and a call made while
    recording is disabled runs at most the plugin layers: neither reserves a key of the stream."""

    @classmethod
    def setUpClass(cls) -> None:
        cls.body = extract_wrapper_body(
            render_commands([registry_command('glIsTexture')], frozenset()), 'glIsTexture')

    def test_nested_call_returns_before_reserving_a_key(self) -> None:
        early = self.body.find('if (guard.IsNested() || !RecordingEnabled()) {')
        driver = self.body.find('    return realFunction(texture);\n  }', early)
        key = self.body.find('CaptureManager::Get().CreateCommandKey()')
        self.assertNotEqual(early, -1, self.body)
        self.assertNotEqual(driver, -1, self.body)
        self.assertLess(driver, key)

    def test_disabled_recording_runs_plugin_layers_under_a_plugin_key(self) -> None:
        plugins = self.body.find('if (!guard.IsNested() && PluginLayersLoaded()) {')
        plugin_key = self.body.find(
            'command.m_Key = CaptureManager::Get().CreatePluginCommandKey();')
        pre = self.body.find('preLayer->Pre(command);')
        post = self.body.find('postLayer->Post(command);')
        key = self.body.find('CaptureManager::Get().CreateCommandKey()')
        self.assertNotEqual(plugins, -1, self.body)
        self.assertLess(plugins, plugin_key)
        self.assertLess(plugin_key, pre)
        self.assertLess(pre, post)
        self.assertLess(post, key)


class MappedWritePublisherTests(unittest.TestCase):
    """Writes through coherent maps and to imported Vulkan memory reach the stream only before
    these commands, so a reader left out would replay with stale memory, and the written pages
    published after a writer left out would overwrite what it wrote."""

    @classmethod
    def setUpClass(cls) -> None:
        names = ('glDrawArrays', 'glMultiDrawElementsIndirect', 'glDrawTransformFeedback',
                 'glDispatchCompute', 'glDispatchComputeIndirect', 'glCopyBufferSubData',
                 'glCopyNamedBufferSubData', 'glGetBufferSubData', 'glGetNamedBufferSubData',
                 'glCopyImageSubData', 'glBlitFramebuffer', 'glBlitNamedFramebuffer',
                 'glReadPixels', 'glReadnPixels', 'glTexSubImage2D', 'glTextureSubImage3D',
                 'glCompressedTexImage2D', 'glMultiTexSubImage2DEXT', 'glGetTexImage',
                 'glGetTextureSubImage', 'glGetnCompressedTexImage', 'glGenerateMipmap',
                 'glGenerateTextureMipmap', 'glSignalSemaphoreEXT', 'glWaitSemaphoreEXT',
                 'glDrawBuffer', 'glDrawBuffers', 'glDrawBuffersARB', 'glMapBufferRange',
                 'glFlushMappedBufferRange', 'glUnmapBuffer', 'glBufferSubData', 'glBindBuffer',
                 'glTexParameteri', 'glGetTexParameteriv', 'wglSwapBuffers')
        cls.publishers = mapped_write_publishers([fake_command(name) for name in names])

    def test_copies_from_buffers_textures_and_framebuffers(self) -> None:
        names = ('glNamedCopyBufferSubDataEXT', 'glMulticastCopyBufferSubDataNV',
                 'glCopyTexImage1D', 'glCopyTexImage2DEXT', 'glCopyTexSubImage2D',
                 'glCopyTexSubImage3DOES', 'glCopyTextureImage2DEXT', 'glCopyTextureSubImage1D',
                 'glCopyTextureSubImage3DEXT', 'glCopyMultiTexImage1DEXT',
                 'glCopyMultiTexSubImage3DEXT', 'glCopyPixels', 'glCopyColorTableSGI',
                 'glCopyColorSubTableEXT', 'glCopyConvolutionFilter2D')
        self.assertEqual(mapped_write_publishers([fake_command(name) for name in names]),
                         frozenset(names))

    def test_pixel_unpacks_that_may_read_a_buffer(self) -> None:
        names = ('glBitmap', 'glPolygonStipple', 'glCompressedMultiTexImage2DEXT',
                 'glCompressedMultiTexSubImage3DEXT', 'glColorTable', 'glColorTableSGI',
                 'glColorSubTableEXT', 'glConvolutionFilter1D', 'glConvolutionFilter2DEXT',
                 'glSeparableFilter2D', 'glPixelMapfv', 'glPixelMapuiv', 'glPixelMapusv')
        self.assertEqual(mapped_write_publishers([fake_command(name) for name in names]),
                         frozenset(names))

    def test_vulkan_semaphore_signal_and_wait(self) -> None:
        names = ('glSignalVkSemaphoreNV', 'glWaitVkSemaphoreNV')
        self.assertEqual(mapped_write_publishers([fake_command(name) for name in names]),
                         frozenset(names))

    def test_commands_that_read_no_buffer_or_texture_memory(self) -> None:
        names = ('glColorTableParameterfv', 'glConvolutionParameteri', 'glGetPolygonStipple',
                 'glGetPixelMapfv', 'glPixelStorei', 'glPixelTransferf', 'glLineStipple',
                 'glImportSemaphoreWin32HandleEXT', 'glGenSemaphoresEXT', 'glCreateSemaphoresNV')
        self.assertEqual(mapped_write_publishers([fake_command(name) for name in names]),
                         frozenset())

    def test_accesses_of_buffer_and_texture_memory(self) -> None:
        self.assertEqual(self.publishers, frozenset({
            'glDrawArrays', 'glMultiDrawElementsIndirect', 'glDrawTransformFeedback',
            'glDispatchCompute', 'glDispatchComputeIndirect', 'glCopyBufferSubData',
            'glCopyNamedBufferSubData', 'glGetBufferSubData', 'glGetNamedBufferSubData',
            'glCopyImageSubData', 'glBlitFramebuffer', 'glBlitNamedFramebuffer', 'glReadPixels',
            'glReadnPixels', 'glTexSubImage2D', 'glTextureSubImage3D', 'glCompressedTexImage2D',
            'glMultiTexSubImage2DEXT', 'glGetTexImage', 'glGetTextureSubImage',
            'glGetnCompressedTexImage', 'glGenerateMipmap', 'glGenerateTextureMipmap',
            'glSignalSemaphoreEXT', 'glWaitSemaphoreEXT', 'glBufferSubData'}))

    def test_writers_of_buffer_and_texture_memory(self) -> None:
        names = ('glBufferSubData', 'glBufferSubDataARB', 'glNamedBufferSubData',
                 'glNamedBufferSubDataEXT', 'glClearBufferData', 'glClearBufferSubData',
                 'glClearNamedBufferData', 'glClearNamedBufferSubData',
                 'glClearNamedBufferDataEXT', 'glClearNamedBufferSubDataEXT', 'glClearTexImage',
                 'glClearTexSubImage', 'glClearTexImageEXT', 'glClearTexSubImageEXT', 'glClear',
                 'glClearBufferiv', 'glClearBufferuiv', 'glClearBufferfv', 'glClearBufferfi',
                 'glClearNamedFramebufferiv', 'glClearNamedFramebufferuiv',
                 'glClearNamedFramebufferfv', 'glClearNamedFramebufferfi')
        self.assertEqual(mapped_write_publishers([fake_command(name) for name in names]),
                         frozenset(names))

    def test_clear_values_and_buffer_allocations_are_excluded(self) -> None:
        # These set the values a clear writes, or give a buffer new storage that no published page
        # can overlap.
        names = ('glClearColor', 'glClearDepth', 'glClearDepthf', 'glClearStencil',
                 'glClearIndex', 'glClearAccum', 'glClearColorIiEXT', 'glClearColorIuiEXT',
                 'glBufferData', 'glNamedBufferData', 'glBufferStorage', 'glNamedBufferStorage')
        self.assertEqual(mapped_write_publishers([fake_command(name) for name in names]),
                         frozenset())

    def test_custom_wrappers_are_excluded(self) -> None:
        # A generated wrapper replaced by a custom one never runs.
        source = (CODEGEN_DIR.parent / 'recorder' / 'mapTrackingWrappers.cpp').read_text(
            encoding='utf-8')
        table = source[source.index('CUSTOM_WRAPPERS[]'):]
        custom = frozenset(re.findall(r'\{"(gl\w+)"', table))
        self.assertIn('glFlushMappedBufferRange', custom)
        self.assertEqual(mapped_write_publishers([fake_command(name) for name in custom]),
                         frozenset())

    def test_query_results_written_into_a_buffer(self) -> None:
        # A query buffer object always receives the result of these.
        names = ('glGetQueryBufferObjectiv', 'glGetQueryBufferObjectuiv',
                 'glGetQueryBufferObjecti64v', 'glGetQueryBufferObjectui64v')
        self.assertEqual(mapped_write_publishers([fake_command(name) for name in names]),
                         frozenset(names))
        self.assertEqual(query_buffer_publishers([fake_command(name) for name in names]),
                         frozenset())

    def test_query_results_written_into_a_bound_query_buffer(self) -> None:
        # These write into client memory unless a GL_QUERY_BUFFER is bound.
        names = ('glGetQueryObjectiv', 'glGetQueryObjectuiv', 'glGetQueryObjecti64v',
                 'glGetQueryObjectui64v', 'glGetQueryObjectivARB', 'glGetQueryObjectuivARB',
                 'glGetQueryObjecti64vEXT', 'glGetQueryObjectui64vEXT')
        self.assertEqual(query_buffer_publishers([fake_command(name) for name in names]),
                         frozenset(names))
        self.assertEqual(mapped_write_publishers([fake_command(name) for name in names]),
                         frozenset())
        others = ('glGetQueryiv', 'glGetQueryIndexediv', 'glBeginQuery', 'glQueryCounter')
        self.assertEqual(query_buffer_publishers([fake_command(name) for name in others]),
                         frozenset())

    def test_query_results_publish_only_into_a_bound_query_buffer(self) -> None:
        rendered = render_commands([fake_command('glGetQueryObjectuiv')], frozenset(),
                                   query_buffer_publishers=frozenset({'glGetQueryObjectuiv'}))
        body = extract_wrapper_body(rendered, 'glGetQueryObjectuiv')
        bound = body.index('if (QueryBufferIsBound()) {')
        publish = body.index('MapTrackingService::Get().PublishMappedWrites();')
        self.assertLess(bound, publish)
        self.assertLess(publish, body.index('CaptureManager::Get().CreateCommandKey()'))

    def test_uploads_are_prepared_before_writes_are_published(self) -> None:
        hook = BufferContentHook(call='OnSubData(data)', guard='size >= 0',
                                 prepare='BeforeSubData(data)')
        rendered = render_commands([fake_command('glBufferSubData')], frozenset(),
                                   mapped_write_publishers=frozenset({'glBufferSubData'}),
                                   buffer_content_hooks={'glBufferSubData': hook})
        body = extract_wrapper_body(rendered, 'glBufferSubData')
        prepare = body.index('BufferShadowService::Get().BeforeSubData(data);')
        self.assertLess(body.index('if (size >= 0) {'), prepare)
        self.assertLess(prepare, body.index('MapTrackingService::Get().PublishMappedWrites();'))
        self.assertLess(body.index('CaptureManager::Get().CreateCommandKey()'),
                        body.index('BufferShadowService::Get().OnSubData(data);'))

    def test_deletion_uses_a_copy_of_the_names(self) -> None:
        command = Command(
            name='glDeleteBuffers', api=Api.GL, enabled=False, command_type=CmdType.NONE,
            return_value=ReturnValue(type='void'),
            params=[Parameter(name='n', type='GLsizei'),
                    Parameter(name='buffers', type='const GLuint *')])
        rendered = render_commands(
            [command], frozenset(),
            buffer_content_hooks={'glDeleteBuffers': BufferContentHook(call='OnDelete(n, buffers)')})
        body = extract_wrapper_body(rendered, 'glDeleteBuffers')
        copy = body.index('buffers = deletedBuffers.data();')
        self.assertLess(copy, body.index('MapTrackingService::Get().PublishBeforeDelete(n, buffers);'))
        # The nested call is the first one; the others must follow the copy.
        calls = [match.start() for match in re.finditer(r'realFunction\(n, buffers\);', body)]
        self.assertLess(copy, calls[-1])
        self.assertLess(copy, body.index('MapTrackingService::Get().OnDelete(n, buffers);'))

    def test_writes_are_published_before_the_key_is_reserved(self) -> None:
        rendered = render_commands([fake_command('glDrawArrays'), fake_command('glBindBuffer')],
                                   frozenset(), mapped_write_publishers=frozenset({'glDrawArrays'}))
        body = extract_wrapper_body(rendered, 'glDrawArrays')
        publish = body.index('MapTrackingService::Get().PublishMappedWrites();')
        self.assertLess(publish, body.index('CaptureManager::Get().CreateCommandKey()'))
        self.assertNotIn('PublishMappedWrites',
                         extract_wrapper_body(rendered, 'glBindBuffer'))

    def test_display_list_calls_and_immediate_mode_draws(self) -> None:
        names = ('glCallList', 'glCallLists', 'glBegin', 'glRectd', 'glRectfv', 'glRecti',
                 'glRectsv', 'glEvalMesh1', 'glEvalMesh2')
        self.assertEqual(mapped_write_publishers([fake_command(name) for name in names]),
                         frozenset(names))

    def test_commands_only_valid_between_begin_and_end_are_excluded(self) -> None:
        # Writes the commands between glBegin and glEnd read are published before glBegin.
        names = ('glEnd', 'glVertex3f', 'glArrayElement', 'glEvalCoord2f', 'glEvalPoint1',
                 'glBeginQuery', 'glBeginTransformFeedback', 'glNewList', 'glEndList',
                 'glListBase', 'glGenLists')
        self.assertEqual(mapped_write_publishers([fake_command(name) for name in names]),
                         frozenset())

    def test_begin_and_end_bracket_the_publication(self) -> None:
        self.assertEqual(PRIMITIVE_PAIR_COMMANDS, {'glBegin': True, 'glEnd': False})
        rendered = render_commands(
            [fake_command('glBegin'), fake_command('glEnd'), fake_command('glCallList')],
            frozenset(), mapped_write_publishers=frozenset({'glBegin', 'glCallList'}),
            primitive_pair_commands=PRIMITIVE_PAIR_COMMANDS)
        # The last realFunction() call is the one past the nested-call early return.
        begin = extract_wrapper_body(rendered, 'glBegin')
        # glBegin publishes before it starts the pair.
        self.assertLess(begin.index('PublishMappedWrites();'), begin.rindex('realFunction();'))
        self.assertLess(begin.rindex('realFunction();'),
                        begin.index('MapTrackingService::Get().SetInPrimitivePair(true);'))
        end = extract_wrapper_body(rendered, 'glEnd')
        self.assertLess(end.rindex('realFunction();'),
                        end.index('MapTrackingService::Get().SetInPrimitivePair(false);'))
        self.assertNotIn('SetInPrimitivePair', extract_wrapper_body(rendered, 'glCallList'))


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


class RecorderWrapperPartsTests(unittest.TestCase):
    """The recorder wrappers are generated into RECORDER_WRAPPER_PARTS files whose registration
    functions all feed the lookup in part 0."""

    @classmethod
    def setUpClass(cls) -> None:
        # Two more than a multiple of the part count, so that the parts differ in size.
        cls.names = [f'wglFakeCommand{index:02}' for index in range(RECORDER_WRAPPER_PARTS * 3 + 2)]
        cls.parts = mako_render_parts(TEMPLATE_PATH, RECORDER_WRAPPER_PARTS,
                                      [fake_command(name) for name in cls.names],
                                      **template_arguments())

    def test_parts_have_contiguous_commands_of_nearly_equal_counts(self) -> None:
        registered = [re.findall(r'\{"(\w+)", reinterpret_cast<void\*>\(&Wrapper_\1\)\},', part)
                      for part in self.parts]
        self.assertEqual([len(names) for names in registered],
                         [4, 4] + [3] * (RECORDER_WRAPPER_PARTS - 2))
        self.assertEqual([name for names in registered for name in names], self.names)

    def test_every_command_is_defined_and_registered_exactly_once(self) -> None:
        for name in self.names:
            with self.subTest(command=name):
                definitions = [part.count(f'APIENTRY Wrapper_{name}(') for part in self.parts]
                registrations = [part.count(f'{{"{name}", ') for part in self.parts]
                self.assertEqual(sum(definitions), 1)
                self.assertEqual(definitions, registrations)

    def test_each_part_defines_its_own_registration_function(self) -> None:
        for index, part in enumerate(self.parts):
            with self.subTest(part=index):
                self.assertEqual(
                    re.findall(r'^void AddRecorderWrappers(\d+)\(.*\) \{$', part, re.MULTILINE),
                    [str(index)])

    def test_only_part_0_defines_the_lookup(self) -> None:
        definition = 'void* GetGeneratedRecorderWrapper(const char* name) {'
        self.assertEqual([part.count(definition) for part in self.parts],
                         [1] + [0] * (RECORDER_WRAPPER_PARTS - 1))

    def test_the_lookup_registers_the_wrappers_of_all_parts(self) -> None:
        lookup = self.parts[0][self.parts[0].index('void* GetGeneratedRecorderWrapper('):]
        self.assertIn(f'map.reserve({len(self.names)});', lookup)
        self.assertEqual(re.findall(r'AddRecorderWrappers(\d+)\(map\);', lookup),
                         [str(index) for index in range(RECORDER_WRAPPER_PARTS)])
        declarations = re.findall(
            r'^void AddRecorderWrappers(\d+)\(std::unordered_map<std::string_view, void\*>& '
            r'nameToWrapper\);$', self.parts[0], re.MULTILINE)
        self.assertEqual(declarations, [str(index) for index in range(1, RECORDER_WRAPPER_PARTS)])

    def test_a_part_without_commands_is_rejected(self) -> None:
        commands = [fake_command(name) for name in self.names[:RECORDER_WRAPPER_PARTS - 1]]
        with self.assertRaises(ValueError):
            mako_render_parts(TEMPLATE_PATH, RECORDER_WRAPPER_PARTS, commands,
                              **template_arguments())

    def test_the_build_lists_every_part(self) -> None:
        expected = [f'recorderWrappers{index}Auto.cpp' for index in range(RECORDER_WRAPPER_PARTS)]
        for cmake_lists in (CODEGEN_DIR / 'CMakeLists.txt',
                            CODEGEN_DIR.parent / 'recorder' / 'CMakeLists.txt'):
            with self.subTest(file=str(cmake_lists)):
                self.assertEqual(
                    re.findall(r'recorderWrappers\d+Auto\.cpp', cmake_lists.read_text()), expected)


if __name__ == '__main__':
    unittest.main()
