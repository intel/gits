#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

import re
from dataclasses import dataclass

from buffer_content_hooks import buffer_content_hooks
from client_array_draws import client_array_draws, client_array_setters
from generator_helpers import mako_write, mako_write_parts
from handle_lifecycle import (
    CONTEXT_SHARE_PARAM,
    FRAME_END_COMMANDS,
    HANDLE_CREATING_COMMANDS,
    HANDLE_PARAM_TO_ERASE_ON_SUCCESS,
    MAPPED_HANDLE_SERVICE_NAMES,
    SHARE_LISTS_COMMANDS,
)
from intermediates import Command

# HANDLE_CREATING_COMMANDS/HANDLE_PARAM_TO_ERASE_ON_SUCCESS are already unioned across every
# mapped-handle type (HGLRC, GLsync, ...) by handle_lifecycle.py itself - see that module's own
# header comment for why a single generic table, not a per-consumer union of per-type modules,
# is the right shape now that a second mapped-handle type has actually shown up.

# Commands whose Wrapper_<cmd> must reserve its ordering key TOGETHER with a second, "spare"
# key - via CaptureManager::CreateCommandKeyPair(), one atomic fetch_add(2), not two separate
# CreateCommandKey() calls - rather than the single CreateCommandKey() every other command
# reserves. Mostly WindowGeometryTrackingLayer's hooked WGL commands (see that
# layer's own header comment, apis/OpenGL/recorder/include/windowGeometryTrackingLayer.h): each may
# need to record a SEPARATE, synthetic WindowGeometryMetaCommand that must decode/replay
# strictly BEFORE the hooked call itself, and the only race-free way to guarantee that ordering
# is to reserve both keys as one indivisible unit at the SAME wrapper-entry point a single
# CreateCommandKey() call would otherwise occupy - see CreateCommandKeyPair()'s own header
# comment (apis/OpenGL/recorder/include/captureManager.h) for why reserving them separately, at two
# different points in time (the pattern this table replaced), cannot give the same guarantee.
#
# recorderWrappersAuto.cpp.mako's own header comment covers the exact generated shape; the
# short version: for a command in this set, the spare (first, smaller) key of the pair is
# handed to CaptureManager::SetPendingPreHookKey() immediately, on the same thread, before this
# wrapper's own Pre layer loop runs - WindowGeometryTrackingLayer::Pre() is the only consumer,
# via TakePendingPreHookKey() - and the second (larger) key becomes this wrapper's own
# `orderingKey`, used exactly like every other command's single reserved key from here on.
#
# wglCreatePbufferARB/EXT, glXCreateContext and the GLX commands taking a GLXFBConfig, except
# glXCreateWindow and glXCreatePixmap, are the other entries: PixelFormatCaptureLayer::Pre()
# records their PixelFormatMetaCommand (pixelFormatMetaTypes.h) under the spare key.
# glXCreateWindow samples its X window, like wglSetPixelFormat, and glXCreatePixmap and
# glXCreateGLXPixmap their X pixmap. wglMakeContextCurrentARB/EXT, glXMakeContextCurrent and
# glXMakeCurrentReadSGI record one geometry sample per drawable, both in the metacommand recorded
# under the spare key.
#
# A plain Python set, not a dict, unlike CONTEXT_SHARE_PARAM/SHARE_LISTS_COMMANDS above:
# membership alone is everything the template needs here (there is no per-command payload to
# carry - every entry is handled completely uniformly).
PRE_HOOK_KEY_PAIR_COMMANDS: frozenset[str] = frozenset({
    'wglSetPixelFormat', 'wglCreateContext', 'wglCreateContextAttribsARB', 'wglMakeCurrent',
    'wglSwapBuffers', 'wglSwapLayerBuffers', 'wglCreatePbufferARB', 'wglCreatePbufferEXT',
    'wglMakeContextCurrentARB', 'wglMakeContextCurrentEXT',
    'glXMakeCurrent', 'glXMakeContextCurrent', 'glXMakeCurrentReadSGI', 'glXSwapBuffers',
    'glXGetVisualFromFBConfig', 'glXCreateWindow', 'glXCreatePbuffer', 'glXCreateContext',
    'glXCreateNewContext', 'glXCreateContextAttribsARB', 'glXCreatePixmap', 'glXCreateGLXPixmap',
})


# Commands whose Wrapper_<cmd> reserves a key pair whose SECOND key goes to a Post layer, for a
# metacommand that must replay right after the hooked call with no other thread's command in
# between - see CaptureManager::SetPendingPostHookKey() (captureManager.h). Exclusively
# UniformArrayCaptureLayer's glGetUniformLocation(ARB) today: a glUniform* on another thread using
# a computed array-element location must not replay before the UniformArrayMetaCommand that maps
# it.
POST_HOOK_KEY_PAIR_COMMANDS: frozenset[str] = frozenset({
    'glGetUniformLocation', 'glGetUniformLocationARB',
})


# PRE_HOOK_KEY_PAIR_COMMANDS whose Wrapper_<cmd> reserves a third key, ahead of the pair, and hands
# it to the CaptureManager setter named here:
# - MappedTextureLayer's MappedTextureDataMetaCommand: mapped-texture writes the application
#   publishes by swapping must replay before the swap presents them.
# - PixelFormatCaptureLayer's PixelFormatMetaCommand: the window's pixel format attributes, which
#   its PIXELFORMATDESCRIPTOR cannot carry, must replay before the format is set, and the
#   framebuffer configuration or visual of a GLX window or pixmap before it is created.
PRE_HOOK_EXTRA_KEY_COMMANDS: dict[str, str] = {
    'wglSwapBuffers': 'SetPendingMappedTextureKey',
    'wglSwapLayerBuffers': 'SetPendingMappedTextureKey',
    'glXSwapBuffers': 'SetPendingMappedTextureKey',
    'wglSetPixelFormat': 'SetPendingPixelFormatKey',
    'glXCreateWindow': 'SetPendingPixelFormatKey',
    'glXCreatePixmap': 'SetPendingPixelFormatKey',
    'glXCreateGLXPixmap': 'SetPendingPixelFormatKey',
}
assert PRE_HOOK_EXTRA_KEY_COMMANDS.keys() <= PRE_HOOK_KEY_PAIR_COMMANDS
# A frame end's wrapper reserves its keys with CaptureManager::CreateFrameEndKeyRange(), which
# only the PRE_HOOK_EXTRA_KEY_COMMANDS reservation calls.
assert FRAME_END_COMMANDS <= PRE_HOOK_EXTRA_KEY_COMMANDS.keys()


# Commands that may read or write buffer or texture memory: draws, including display list calls,
# glBegin and the other immediate mode draws, dispatches, buffer copies, reads, uploads and
# clears, texture uploads, copies, readbacks and clears, framebuffer clears, copies and blits,
# mipmap generation, the other pixel unpacks a GL_PIXEL_UNPACK_BUFFER may source, query results
# written into a buffer, and the semaphore signals and waits. Their Wrapper_<cmd> calls
# MapTrackingService::PublishMappedWrites() before reserving its key, so writes through coherent
# and implicit-flush persistent maps and host writes to memory GL imports from Vulkan replay before the command accesses the
# memory, and the pages they wrote are not published again over what it wrote. Buffer maps and flushes have custom wrappers
# (recorder/mapTrackingWrappers.cpp), which these patterns exclude.
_MAPPED_WRITE_PUBLISHER_RE = re.compile(
    r'^gl(?:Multi)?Draw(?!Buffers?(?:ARB|ATI|EXT|NV|IndexedEXT)?$)'
    r'|^glCallLists?$'
    r'|^glBegin$'
    r'|^glRect[dfis]v?$'
    r'|^glEvalMesh[12]$'
    r'|^glDispatchCompute'
    r'|^gl(?:Copy|Get)(?:Named)?Buffer(?:Sub)?Data'
    r'|^gl(?:Named)?BufferSubData'
    r'|^glClear(?:Named)?Buffer(?:Sub)?Data'
    r'|^glClearTex(?:Sub)?Image'
    r'|^glClear$'
    r'|^glClear(?:Buffer|NamedFramebuffer)(?:iv|uiv|fv|fi)$'
    r'|^gl(?:Named|Multicast)CopyBufferSubData'
    r'|^glCopyImageSubData'
    r'|^glBlit(?:Named)?Framebuffer'
    r'|^glReadn?Pixels'
    r'|^gl(?:Multi)?(?:Compressed)?(?:Multi)?Tex(?:ture)?(?:Sub)?Image[123]D'
    r'|^glCopy(?:Multi)?Tex(?:ture)?(?:Sub)?Image[123]D'
    r'|^glCopy(?:Pixels|Color(?:Sub)?Table|ConvolutionFilter[12]D)'
    r'|^glGetn?(?:Compressed)?(?:Multi)?Tex(?:ture)?(?:Sub)?Image'
    r'|^glGenerate(?:Texture|MultiTex)?Mipmap'
    r'|^gl(?:Bitmap|PolygonStipple)'
    r'|^gl(?:Color(?:Sub)?Table|ConvolutionFilter[12]D|SeparableFilter2D)(?:EXT|SGI)?$'
    r'|^glPixelMap(?:fv|uiv|usv)$'
    r'|^glGetQueryBufferObject(?:i|ui|i64|ui64)v$'
    r'|^gl(?:Signal|Wait)(?:Vk)?Semaphore')


def mapped_write_publishers(commands: list[Command]) -> frozenset[str]:
    """The commands before which mapped writes are published."""
    return frozenset(cmd.name for cmd in commands if _MAPPED_WRITE_PUBLISHER_RE.search(cmd.name))


# Commands that write a query result into the GL_QUERY_BUFFER when one is bound, and into client
# memory otherwise. Their Wrapper_<cmd> publishes mapped writes as mapped_write_publishers() do,
# but only when a query buffer is bound, as applications poll query results.
_QUERY_BUFFER_PUBLISHER_RE = re.compile(r'^glGetQueryObject(?:i|ui|i64|ui64)v(?:ARB|EXT)?$')


def query_buffer_publishers(commands: list[Command]) -> frozenset[str]:
    """The commands before which mapped writes are published when a query buffer is bound."""
    return frozenset(cmd.name for cmd in commands
                     if _QUERY_BUFFER_PUBLISHER_RE.search(cmd.name))


# Whether each command starts or ends a glBegin/glEnd pair. Its Wrapper_<cmd> tells
# MapTrackingService after the call, as PublishMappedWrites() does nothing between them: the
# commands it records replay GL calls that are invalid there, such as before a glCallList, and
# glBegin published the writes the pair reads.
PRIMITIVE_PAIR_COMMANDS: dict[str, bool] = {'glBegin': True, 'glEnd': False}


def pre_hook_key_pair_commands(commands: list[Command]) -> frozenset[str]:
    """PRE_HOOK_KEY_PAIR_COMMANDS plus ClientArraysLayer's draws (client_array_draws.py), which
    record a ClientArraysMetaCommand under the spare key the same way. The two sets must not
    overlap: the pending key has a single consumer per call."""
    draws = frozenset(client_array_draws(commands))
    overlap = draws & PRE_HOOK_KEY_PAIR_COMMANDS
    if overlap:
        raise ValueError(f"pre-hook key pair commands with two consumers: {sorted(overlap)}")
    pre_hook = PRE_HOOK_KEY_PAIR_COMMANDS | draws
    if pre_hook & POST_HOOK_KEY_PAIR_COMMANDS:
        raise ValueError("commands with both a pre-hook and a post-hook key pair: "
                         f"{sorted(pre_hook & POST_HOOK_KEY_PAIR_COMMANDS)}")
    return pre_hook


# The recorder wrappers are split into this many files. Changing it also requires changing the
# recorderWrappersNAuto.cpp lists in apis/OpenGL/codegen/CMakeLists.txt and
# apis/OpenGL/recorder/CMakeLists.txt, which the build does not derive.
RECORDER_WRAPPER_PARTS = 4

# GLX drawables and the X windows and pixmaps they are created for, which EncoderLayer records as
# their GlxDrawableIdentityService identities (recorder/include/glxDrawableIdentityService.h).
GLX_DRAWABLE_BASE_TYPES: frozenset[str] = frozenset({
    'GLXDrawable', 'GLXWindow', 'GLXPbuffer', 'GLXPixmap', 'Window', 'Pixmap',
})


@dataclass(frozen=True)
class DrawableIdentities:
    """The drawable params and return value EncoderLayer records as identities, and the
    expression of the display they are on."""
    display: str
    params: tuple[str, ...]
    returns: bool


def drawable_identities(commands: list[Command]) -> dict[str, DrawableIdentities]:
    """The enabled commands naming a drawable. A command without a display, such as
    glXGetCurrentDrawable, names a drawable on the display of the current context."""
    result: dict[str, DrawableIdentities] = {}
    for cmd in commands:
        if not cmd.enabled:
            continue
        arguments = [*cmd.params, cmd.return_value]
        pointers = [p.name for p in arguments if p.type_facts.pointer_depth > 0 and
                    p.type_facts.base_type in GLX_DRAWABLE_BASE_TYPES]
        if pointers:
            raise ValueError(f"{cmd.name}: drawable pointers are not recorded as identities: "
                             f"{pointers}")
        params = tuple(p.name for p in cmd.params
                       if p.type_facts.base_type in GLX_DRAWABLE_BASE_TYPES)
        returns = cmd.return_value.type_facts.base_type in GLX_DRAWABLE_BASE_TYPES
        if not params and not returns:
            continue
        displays = [p.name for p in cmd.params
                    if p.type_facts.base_type == 'Display' and p.type_facts.pointer_depth == 1]
        if len(displays) > 1:
            raise ValueError(f"{cmd.name}: drawables of more than one display: {displays}")
        display = (f'command.m_{displays[0]}.Value' if displays else
                   'g_DispatchTable.glXGetCurrentDisplay.load(std::memory_order_acquire)()')
        result[cmd.name] = DrawableIdentities(display, params, returns)
    return result


def generate_recorder_files(commands: list[Command]) -> None:
    """Generate capture layers and forwarding wrappers for interceptor commands."""
    draws = client_array_draws(commands)
    mako_write('templates/clientArraysLayerAuto.h.mako',
               'recorder/clientArraysLayerAuto.h', client_array_draws=draws)
    mako_write('templates/clientArraysLayerAuto.cpp.mako',
               'recorder/clientArraysLayerAuto.cpp', client_array_draws=draws)
    mako_write_parts('templates/recorderWrappersAuto.cpp.mako',
                     'recorder/recorderWrappers{}Auto.cpp', RECORDER_WRAPPER_PARTS, commands,
                     handle_creating_commands=HANDLE_CREATING_COMMANDS,
                     handle_param_to_erase_on_success=HANDLE_PARAM_TO_ERASE_ON_SUCCESS,
                     mapped_handle_service_names=MAPPED_HANDLE_SERVICE_NAMES,
                     context_share_param=CONTEXT_SHARE_PARAM,
                     share_lists_commands=SHARE_LISTS_COMMANDS,
                     pre_hook_key_pair_commands=pre_hook_key_pair_commands(commands),
                     post_hook_key_pair_commands=POST_HOOK_KEY_PAIR_COMMANDS,
                     pre_hook_extra_key_commands=PRE_HOOK_EXTRA_KEY_COMMANDS,
                     frame_end_commands=FRAME_END_COMMANDS,
                     mapped_write_publishers=mapped_write_publishers(commands),
                     query_buffer_publishers=query_buffer_publishers(commands),
                     primitive_pair_commands=PRIMITIVE_PAIR_COMMANDS,
                     client_array_setters=client_array_setters(commands),
                     buffer_content_hooks=buffer_content_hooks(commands))
    # EncoderLayer (docs/development/opengl-layer-architecture-alignment-plan.md): the sole,
    # last-registered Post layer, dispatched through by the wrapper above via its
    # GetPostLayers() loop instead of calling CaptureManager::RecordCommand() inline.
    mako_write('templates/encoderLayerAuto.h.mako',
               'recorder/encoderLayerAuto.h', commands=commands)
    mako_write('templates/encoderLayerAuto.cpp.mako',
               'recorder/encoderLayerAuto.cpp', commands=commands,
               frame_end_commands=FRAME_END_COMMANDS,
               drawable_identities=drawable_identities(commands))