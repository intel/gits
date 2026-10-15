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
    # Registers the real, replay-time location the driver just returned into
    # UniformLocationMapService, keyed by the captured location this call's Command originally
    # decoded - see that service's own header comment and
    # modern-opengl-backend-architecture.md's uniform-location decision record.
    'glGetUniformLocation': 'GlGetUniformLocationRunner',
    # Tracks per-context "current program" in UniformLocationMapService after the real call -
    # see that service's own header comment for why (classic glUniform*() calls have no
    # program parameter of their own to resolve against). Previously a `% if cmd.name ==
    # 'glUseProgram':` branch spliced directly into commandRunnersAuto.cpp.mako's shared
    # per-command loop - moved here for the same reason Decision 11 moved the original three
    # (a hardcoded per-command-name branch in a shared generated template, rather than in this
    # table of hand-written exceptions).
    'glUseProgram': 'GlUseProgramRunner',
    # Grows the player's single skeleton window (created at an arbitrary, hardcoded 640x480
    # fallback default by HdcVirtualizationService::Resolve() - see
    # docs/development/opengl2-player-skeleton-plan.md's Decision 1) to fit the app's real
    # rendering resolution - see GlViewportRunner::Run()'s own comment and
    # HdcVirtualizationService::GrowToFit()'s header comment for the full rationale.
    #
    # DEFENSIVE FALLBACK ONLY: this backend now captures the app's REAL Win32 window geometry
    # at record time (WindowGeometryTrackingService, windowGeometryMetaTypes.h's
    # WindowGeometryMetaCommand), so a stream recorded after that feature normally reaches
    # Resolve() with the correct size already known and this glViewport-inferred substitute
    # signal never fires a visible resize for it. Still needed - hence this custom runner
    # staying excluded from the generic per-command codegen loop - for a stream recorded before
    # that capture existed, or one where WindowFromDC/GetClientRect failed at capture time; not
    # because glViewport remains the only size signal available.
    'glViewport': 'GlViewportRunner',
}
