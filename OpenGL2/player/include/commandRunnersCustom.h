// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

// Hand-written player-side CommandRunner subclasses for the commands whose real replay behavior
// can't be expressed by commandRunnersAuto.cpp.mako's fully generic Run() body - see
// docs/development/opengl2-player-skeleton-plan.md's Decision 11 and
// codegen/custom_player_runners.py's CUSTOM_PLAYER_RUNNERS table for why this file exists and
// how openGL2CommandFactoryAuto.cpp routes these command IDs here instead of to their generated
// (but otherwise unused) commandRunnersAuto.h/.cpp counterparts - that table, not a count here,
// is the authoritative, always-accurate list of which commands this file covers. Mirrors
// recorder/include/mapTrackingWrappers.h's "small, hand-written, not generated" precedent,
// applied to player-side command *execution* instead of recorder-side wrapper *dispatch*.
//
// Each class has the exact same DecodeCommand()/private-member shape as its
// commandRunnersAuto.h.mako-generated counterpart of the same command name - only Run()'s body
// differs.

#include "commandCodersAuto.h"
#include "commandRunner.h"
#include "commandsAuto.h"

namespace gits::opengl2 {

// wglCreateContext's Run() - see docs/development/opengl2-player-skeleton-plan.md's Decision 9:
// a defensive EnsurePixelFormatSet(nullptr) fallback runs before the real call, for an app that
// never called (or that called an unsupported variant of) SetPixelFormat before creating its
// context. A no-op if a real, replayed wglSetPixelFormat command (see WglSetPixelFormatRunner
// below) already set one.
class WglCreateContextRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  wglCreateContextCommand command;
};

// wglCreateContextAttribsARB's Run() - same Decision 9 defensive pixel-format fallback as
// WglCreateContextRunner above.
class WglCreateContextAttribsARBRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  wglCreateContextAttribsARBCommand command;
};

// wglMakeCurrent's Run() - also triggers player::WarmUpResolvableFunctions() on every successful
// call that attaches a context (see docs/development/opengl2-player-skeleton-plan.md's
// Decision 8), though that function itself now skips its actual resolution pass for a context
// it has already warmed up before, keyed by the context's stable GITSKey rather than "the first
// time on this thread" - see WarmUpResolvableFunctions's own declaration comment
// (openglDriversAuto.h) for the full per-context caching rationale.
class WglMakeCurrentRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  wglMakeCurrentCommand command;
};

// wglSetPixelFormat's Run() - re-negotiates the captured PIXELFORMATDESCRIPTOR request instead
// of replaying the recorded, driver-specific format index verbatim, see
// docs/development/opengl2-player-skeleton-plan.md's Decision 9.
class WglSetPixelFormatRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  wglSetPixelFormatCommand command;
};

// glGetUniformLocation's Run() - see docs/development/modern-opengl-backend-architecture.md's
// uniform-location decision record and common/services/uniformLocationMapService.h: registers
// the real, replay-time location the driver just returned under the captured (recorder-time)
// location this call's Command originally decoded, so any later glUniform*(location, ...)
// command can resolve through UniformLocationMapService::Resolve() instead of replaying the
// captured integer verbatim.
class GlGetUniformLocationRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  glGetUniformLocationCommand command;
};

// glUseProgram's Run() - see common/services/uniformLocationMapService.h's header comment:
// classic (non-DSA) glUniform*() calls carry no program parameter of their own, so
// UniformLocationMapService must track "current program" itself, per-context, updated at the
// one call that changes it - CurrentContext() (handleArgumentUpdaters.h) is only well-defined
// once the generic Run() body's usual resolve/dispatch has already happened, exactly like every
// other hand-written runner here, hence a full custom Run() rather than a single extra line
// spliced into the generic template.
class GlUseProgramRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  glUseProgramCommand command;
};

} // namespace gits::opengl2
