#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Player-side command names whose real replay behavior cannot be expressed by
# commandRunnersAuto.cpp.mako's fully generic Run() body (resolve handles, call the real
# dispatch-table function, register created handles - see
# docs/development/opengl2-player-skeleton-plan.md's Decision 6) - see Decision 11 for why this
# table exists and what it replaced (three `% if cmd.name == ...` branches that used to live
# directly inside that template's shared per-command loop).
#
# Mirrors recorder/include/mapTrackingWrappers.h's GetCustomRecorderWrapper()/
# GetGeneratedRecorderWrapper() precedent - a small, hand-written table of real command names
# needing non-generic logic, consulted by codegen instead of baked into a shared generated
# template - applied to player-side command *execution* instead of recorder-side wrapper
# *dispatch*. Unlike that recorder-side mechanism (a runtime priority check), the player's
# CommandFactory dispatches by a single C++ `switch` over CommandId, which cannot have two
# `case` labels for the same enum value - so here the *codegen* itself (not runtime code)
# excludes each of these command names from openGL2CommandFactoryAuto.cpp.mako's generated
# per-command loop and instead emits one hand-written-shaped case (mirroring that same
# template's existing ID_META_UNSUPPORTED_COMMAND/ID_META_MAPPED_DATA cases) returning the
# corresponding class below.
#
# commandRunnersAuto.h.mako/.cpp.mako still generate the usual, fully generic `<name>Runner`
# class/Run() for every command name listed here - exactly like the recorder side's
# Wrapper_glMapBufferRange/Wrapper_glUnmapBuffer, these generated classes become dead code (never
# instantiated by anything, since the factory always routes their CommandId here instead) rather
# than being special-cased out of generation entirely; leaving them generated keeps
# commandRunnersAuto.cpp.mako itself fully name-agnostic, with zero per-command branches.
#
# Maps GL/WGL command name -> the hand-written CommandRunner subclass name (declared/defined in
# player/include/commandRunnersCustom.h / player/commandRunnersCustom.cpp) that replaces its
# generated counterpart.
CUSTOM_PLAYER_RUNNERS: dict[str, str] = {
    'wglSetPixelFormat': 'WglSetPixelFormatRunner',
    'wglCreateContext': 'WglCreateContextRunner',
    'wglCreateContextAttribsARB': 'WglCreateContextAttribsARBRunner',
    'wglMakeCurrent': 'WglMakeCurrentRunner',
    # Bind both DCs to the windows of their geometry samples before resolving either - see
    # HdcVirtualizationService::BindSampledWindows().
    'wglMakeContextCurrentARB': 'WglMakeContextCurrentARBRunner',
    'wglMakeContextCurrentEXT': 'WglMakeContextCurrentEXTRunner',
    # Bind the recorded HDC to the window of the geometry sample taken for this exact swap (an
    # atomic pre-hook-key pair with WindowGeometryTrackingLayer's per-frame poll - see that
    # layer's own header comment) - see HdcVirtualizationService::BindSampledWindow().
    'wglSwapBuffers': 'WglSwapBuffersRunner',
    'wglSwapLayerBuffers': 'WglSwapLayerBuffersRunner',
    # Re-chooses the captured pixel format on the replay driver, and maps recorded pbuffer DCs
    # onto the real ones - see player/include/pbufferService.h.
    'wglCreatePbufferARB': 'WglCreatePbufferARBRunner',
    'wglGetPbufferDCARB': 'WglGetPbufferDCARBRunner',
    'wglReleasePbufferDCARB': 'WglReleasePbufferDCARBRunner',
    'wglDestroyPbufferARB': 'WglDestroyPbufferARBRunner',
    'wglCreatePbufferEXT': 'WglCreatePbufferEXTRunner',
    'wglGetPbufferDCEXT': 'WglGetPbufferDCEXTRunner',
    'wglReleasePbufferDCEXT': 'WglReleasePbufferDCEXTRunner',
    'wglDestroyPbufferEXT': 'WglDestroyPbufferEXTRunner',
}
