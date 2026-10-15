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
// uniform-location decision record and uniformLocationMapService.h: registers
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

// glUseProgram's Run() - see uniformLocationMapService.h's header comment:
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

// glViewport's Run() - beyond the generic real-call forward, also grows the player's single
// skeleton window to fit this call's (width, height) when it targets the window origin - see
// HdcVirtualizationService::GrowToFit()'s header comment for the full rationale.
//
// DEFENSIVE FALLBACK ONLY as of WindowGeometryMetaCommand/WindowGeometryMetaRunner (see
// windowGeometryMetaTypes.h): this backend now captures real Win32 window geometry at record
// time (WindowGeometryTrackingService, mirroring the legacy OpenGL backend's own
// OpenGL/common/wglFunctions.cpp's GetWindowParams()/CwglSetPixelFormat capture, and DX12/
// Vulkan's own CreateWindowMetaCommand-shaped idiom), so a stream recorded after this change
// reaches HdcVirtualizationService::Resolve() with the correct real size already known - this
// glViewport-inferred substitute signal never actually fires a visible resize for such a
// stream (GrowToFit() only grows past whatever SetGeometry() already set, and the real size is
// never smaller than any legitimate full-window glViewport call). Kept, not removed, only for
// a stream recorded BEFORE this change, or one where WindowFromDC/GetClientRect failed at
// capture time.
class GlViewportRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  glViewportCommand command;
};

// wglSwapBuffers's Run() - skips the real driver call entirely when
// HdcVirtualizationService::ShouldSuppressSwap() says so - see that method's own declaration
// comment (hdcVirtualizationService.h) and HdcVirtualizationService::SetGeometry()'s own header
// comment (hdcVirtualizationService.cpp) for the concrete repro and full rationale: a second,
// permanently zero-area window/context in the same recorded process shares this player's one
// real window/HDC with the real main window, and presenting its swaps would otherwise overwrite
// the main window's just-rendered frame on every other swap.
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

// wglSwapLayerBuffers's Run() - see WglSwapBuffersRunner's own comment above; identical
// suppression, same rationale.
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

// glBindTexture's Run() - beyond the generic resolve/dispatch forward, also notes the currently
// active unit with MipmapCompletionService::NoteUnit(), for its draw-time completeness fixup -
// see that service's own header comment (mipmapCompletionService.h) for the full rationale.
// glBindTexture always acts on whichever unit GL_ACTIVE_TEXTURE currently names, unlike
// glBindSampler below (which takes an explicit unit argument), so this queries that first.
class GlBindTextureRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  glBindTextureCommand command;
};

// glBindTextureEXT's Run() - see GlBindTextureRunner's own comment above; identical fixup,
// same rationale (glBindTextureEXT is DSA-free EXT-suffixed glBindTexture, same semantics).
class GlBindTextureEXTRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  glBindTextureEXTCommand command;
};

// glBindTextureUnit's Run() - see GlBindTextureRunner's own comment above; identical fixup, same
// rationale. This is the DSA single-call form (explicit unit argument, no GL_ACTIVE_TEXTURE
// dependency) - a distinct bind entry point from glBindTexture(EXT) above, not a redundant one:
// an application can bind a texture to a unit exclusively through this call and never call
// glBindTexture(EXT) for that texture at all, so this needs its own hook, not reuse of theirs.
class GlBindTextureUnitRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  glBindTextureUnitCommand command;
};

// glBindSampler's Run() - beyond the generic resolve/dispatch forward, also notes the explicit
// unit this call just (re)bound a sampler onto - see that service's own header comment
// (mipmapCompletionService.h) and GlBindTextureRunner's own comment above.
class GlBindSamplerRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  glBindSamplerCommand command;
};

// glNewList's and glEndList's Run() - bracket the compilation MipmapCompletionService records,
// and let it replay GL_COMPILE_AND_EXECUTE as GL_COMPILE followed by an executing glCallList - see
// MipmapCompletionService::NewListMode().
class GlNewListRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  glNewListCommand command;
};

class GlEndListRunner : public stream::CommandRunner {
public:
  void Run() override;

protected:
  void DecodeCommand() override {
    Decode(m_Data, command);
  }

private:
  glEndListCommand command;
};

} // namespace gits::opengl2
