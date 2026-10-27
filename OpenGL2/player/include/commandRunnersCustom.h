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
// a defensive EnsurePixelFormatSet(hdc, nullptr) fallback runs before the real call, for an app
// that never called (or that called an unsupported variant of) SetPixelFormat before creating its
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

// wglMakeContextCurrentARB/EXT's Run() - binds the recorded draw and read DCs to the windows of
// their geometry samples (HdcVirtualizationService::BindSampledWindows()) before resolving either,
// and warms up extension functions as wglMakeCurrent's does.
class WglMakeContextCurrentARBRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  wglMakeContextCurrentARBCommand command;
};

class WglMakeContextCurrentEXTRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  wglMakeContextCurrentEXTCommand command;
};

// wglSetPixelFormat's Run() - re-negotiates the attributes of the PixelFormatMetaCommand replayed
// right before it and the captured PIXELFORMATDESCRIPTOR instead of replaying the recorded,
// driver-specific format index verbatim, see docs/development/opengl2-player-skeleton-plan.md's
// Decision 9.
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

// wglSwapBuffers's Run() - binds the recorded HDC to the window of the geometry sample taken for
// this swap (HdcVirtualizationService::BindSampledWindow()) before presenting that window.
class WglSwapBuffersRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  wglSwapBuffersCommand command;
};

// wglSwapLayerBuffers's Run() - see WglSwapBuffersRunner's own comment above.
class WglSwapLayerBuffersRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  wglSwapLayerBuffersCommand command;
};

// wglCreatePbufferARB's Run() - chooses the pixel format from the attributes of the
// PixelFormatMetaCommand replayed right before it (HdcVirtualizationService) rather than reusing
// the capture driver's format index.
class WglCreatePbufferARBRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  wglCreatePbufferARBCommand command;
};

// wglGetPbufferDCARB's Run() - registers the real DC under the recorded one (PbufferService).
class WglGetPbufferDCARBRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  wglGetPbufferDCARBCommand command;
};

// wglReleasePbufferDCARB's Run() - releases and forgets the real DC registered for the recorded
// one. Skipped for a DC replay never got, which would otherwise resolve to the window's DC.
class WglReleasePbufferDCARBRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  wglReleasePbufferDCARBCommand command;
};

// wglDestroyPbufferARB's Run() - also forgets the pbuffer's DCs the application never released.
class WglDestroyPbufferARBRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  wglDestroyPbufferARBCommand command;
};

// The WGL_EXT_pbuffer twins of the four WGL_ARB_pbuffer runners above, with the same replay.
class WglCreatePbufferEXTRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  wglCreatePbufferEXTCommand command;
};

class WglGetPbufferDCEXTRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  wglGetPbufferDCEXTCommand command;
};

class WglReleasePbufferDCEXTRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  wglReleasePbufferDCEXTCommand command;
};

class WglDestroyPbufferEXTRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  wglDestroyPbufferEXTCommand command;
};

#if !defined(_WIN32)
// glXCreateContext's Run() - registers the context in its share group, as
// WglCreateContextAttribsARBRunner does.
class GlxCreateContextRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  glXCreateContextCommand command;
};

// glXMakeCurrent's Run() - warms up extension functions as WglMakeCurrentRunner does.
class GlxMakeCurrentRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  glXMakeCurrentCommand command;
};
#endif

} // namespace gits::opengl2
