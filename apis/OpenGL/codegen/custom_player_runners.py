#!/usr/bin/python

# ===================== begin_copyright_notice ============================
#
# Copyright (C) 2023-2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#
# ===================== end_copyright_notice ==============================

# Commands whose replay cannot be expressed by commandRunnersAuto.cpp.mako's generic Run() body
# (resolve handles, call the real function, register created handles), mapped to the
# hand-written CommandRunner subclass replacing it (player/include/commandRunnersCustom.h,
# player/commandRunnersCustom.cpp). The CommandFactory dispatches with one switch over
# CommandId, so openGLCommandFactoryAuto.cpp.mako returns the custom runner for these commands
# instead of the generated one. The generated <name>Runner classes are still emitted, unused, so
# commandRunnersAuto.cpp.mako has no per-command branches.
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
    # Register the new context in its share group, and warm up extension functions after
    # make-current, as the WGL runners above do. Commands taking a GLXFBConfig use the one
    # GlxFBConfigService chooses.
    'glXCreateContext': 'GlxCreateContextRunner',
    'glXCreateNewContext': 'GlxCreateNewContextRunner',
    'glXCreateContextAttribsARB': 'GlxCreateContextAttribsARBRunner',
    'glXMakeCurrent': 'GlxMakeCurrentRunner',
    'glXMakeContextCurrent': 'GlxMakeContextCurrentRunner',
    'glXMakeCurrentReadSGI': 'GlxMakeCurrentReadSGIRunner',
    # Defers destroying a context a recorded thread still has current to its release, which
    # ThreadContextService's switches between recorded threads would otherwise do.
    'glXDestroyContext': 'GlxDestroyContextRunner',
    # Register the player's GLX window, pbuffer or pixmap as the recorded one's substitute in
    # GlxDrawableService, and forget it once destroyed. Destroying one a recorded thread still has
    # current is deferred to its release, as for contexts. A GLX pixmap's X pixmap is the player's
    # own, of the recorded size.
    'glXCreateWindow': 'GlxCreateWindowRunner',
    'glXDestroyWindow': 'GlxDestroyWindowRunner',
    'glXCreatePbuffer': 'GlxCreatePbufferRunner',
    'glXDestroyPbuffer': 'GlxDestroyPbufferRunner',
    'glXCreatePixmap': 'GlxCreatePixmapRunner',
    'glXDestroyPixmap': 'GlxDestroyPixmapRunner',
    'glXCreateGLXPixmap': 'GlxCreateGLXPixmapRunner',
    'glXDestroyGLXPixmap': 'GlxDestroyGLXPixmapRunner',
}

# Queries whose generated runner runs its layers but not the real call. Their framebuffer
# configurations are the recorded process's, and nothing on replay reads their results: the
# player chooses its own configurations from the attributes recorded before each command creating
# something with one. The same holds for glXGetConfig's visual, which replays as null when it is
# not one of the mapped ones.
UNREPLAYED_COMMANDS: frozenset[str] = frozenset({
    'glXChooseFBConfig', 'glXGetFBConfigs', 'glXGetFBConfigAttrib', 'glXGetConfig',
})
assert not UNREPLAYED_COMMANDS & CUSTOM_PLAYER_RUNNERS.keys()
