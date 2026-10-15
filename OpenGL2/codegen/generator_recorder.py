#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

from buffer_content_hooks import buffer_content_hooks
from client_array_draws import client_array_draws, client_array_setters
from generator_helpers import mako_write
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
# layer's own header comment, OpenGL2/recorder/include/windowGeometryTrackingLayer.h): each may
# need to record a SEPARATE, synthetic WindowGeometryMetaCommand that must decode/replay
# strictly BEFORE the hooked call itself, and the only race-free way to guarantee that ordering
# is to reserve both keys as one indivisible unit at the SAME wrapper-entry point a single
# CreateCommandKey() call would otherwise occupy - see CreateCommandKeyPair()'s own header
# comment (OpenGL2/recorder/include/captureManager.h) for why reserving them separately, at two
# different points in time (the pattern this table replaced), cannot give the same guarantee.
#
# recorderWrappersAuto.cpp.mako's own header comment covers the exact generated shape; the
# short version: for a command in this set, the spare (first, smaller) key of the pair is
# handed to CaptureManager::SetPendingPreHookKey() immediately, on the same thread, before this
# wrapper's own Pre layer loop runs - WindowGeometryTrackingLayer::Pre() is the only consumer,
# via TakePendingPreHookKey() - and the second (larger) key becomes this wrapper's own
# `orderingKey`, used exactly like every other command's single reserved key from here on.
#
# wglCreatePbufferARB/EXT are the other entries: PixelFormatCaptureLayer::Pre() records their
# PbufferPixelFormatMetaCommand (pbufferPixelFormatMetaTypes.h) under the spare key.
# wglMakeContextCurrentARB/EXT record one geometry sample per DC, both in the metacommand
# recorded under the spare key.
#
# A plain Python set, not a dict, unlike CONTEXT_SHARE_PARAM/SHARE_LISTS_COMMANDS above:
# membership alone is everything the template needs here (there is no per-command payload to
# carry - every entry is handled completely uniformly).
PRE_HOOK_KEY_PAIR_COMMANDS: frozenset[str] = frozenset({
    'wglSetPixelFormat', 'wglCreateContext', 'wglCreateContextAttribsARB', 'wglMakeCurrent',
    'wglSwapBuffers', 'wglSwapLayerBuffers', 'wglCreatePbufferARB', 'wglCreatePbufferEXT',
    'wglMakeContextCurrentARB', 'wglMakeContextCurrentEXT',
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


# PRE_HOOK_KEY_PAIR_COMMANDS whose Wrapper_<cmd> reserves a third key, ahead of the pair, for
# MappedTextureLayer's MappedTextureDataMetaCommand (via CaptureManager::
# SetPendingMappedTextureKey()): mapped-texture writes the application publishes by swapping
# must replay before the swap presents them.
PRE_SWAP_TEXTURE_KEY_COMMANDS: frozenset[str] = frozenset({
    'wglSwapBuffers', 'wglSwapLayerBuffers',
})
assert PRE_SWAP_TEXTURE_KEY_COMMANDS <= PRE_HOOK_KEY_PAIR_COMMANDS


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


def generate_recorder_files(commands: list[Command]) -> None:
    """Generate forwarding wrappers and real-driver dispatch for interceptor commands."""
    draws = client_array_draws(commands)
    mako_write('templates/clientArraysLayerAuto.h.mako',
               'recorder/clientArraysLayerAuto.h', client_array_draws=draws)
    mako_write('templates/clientArraysLayerAuto.cpp.mako',
               'recorder/clientArraysLayerAuto.cpp', client_array_draws=draws)
    mako_write('templates/openglDriversAuto.h.mako',
               'recorder/include/openglDriversAuto.h', commands=commands)
    mako_write('templates/openglDriversAuto.cpp.mako',
               'recorder/openglDriversAuto.cpp', commands=commands)
    mako_write('templates/recorderWrappersAuto.cpp.mako',
               'recorder/recorderWrappersAuto.cpp', commands=commands,
               handle_creating_commands=HANDLE_CREATING_COMMANDS,
               handle_param_to_erase_on_success=HANDLE_PARAM_TO_ERASE_ON_SUCCESS,
               mapped_handle_service_names=MAPPED_HANDLE_SERVICE_NAMES,
               context_share_param=CONTEXT_SHARE_PARAM,
               share_lists_commands=SHARE_LISTS_COMMANDS,
               pre_hook_key_pair_commands=pre_hook_key_pair_commands(commands),
               post_hook_key_pair_commands=POST_HOOK_KEY_PAIR_COMMANDS,
               pre_swap_texture_key_commands=PRE_SWAP_TEXTURE_KEY_COMMANDS,
               client_array_setters=client_array_setters(commands),
               buffer_content_hooks=buffer_content_hooks(commands))
    # EncoderLayer (docs/development/opengl2-layer-architecture-alignment-plan.md): the sole,
    # last-registered Post layer, dispatched through by the wrapper above via its
    # GetPostLayers() loop instead of calling CaptureManager::RecordCommand() inline.
    mako_write('templates/encoderLayerAuto.h.mako',
               'recorder/encoderLayerAuto.h', commands=commands)
    mako_write('templates/encoderLayerAuto.cpp.mako',
               'recorder/encoderLayerAuto.cpp', commands=commands,
               frame_end_commands=FRAME_END_COMMANDS)