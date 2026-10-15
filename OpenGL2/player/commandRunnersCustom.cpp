// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandRunnersCustom.h"
#include "glEnumsAuto.h"
#include "handleArgumentUpdaters.h"
#include "hdcVirtualizationService.h"
#include "layerAuto.h"
#include "openglDriversAuto.h"
#include "playerManager.h"
#include "uniformLocationMapService.h"

namespace gits::opengl2 {

void WglCreateContextRunner::Run() {
  auto& manager = PlayerManager::Get();
  HDC real_hDc = HdcVirtualizationService::Get().Resolve(command.m_hDc.Value);
  // Defensive fallback for an app that never called (or that called an unsupported variant
  // of) SetPixelFormat before creating its context - see
  // docs/development/opengl2-player-skeleton-plan.md's Decision 9. A no-op if a real,
  // replayed wglSetPixelFormat command (see WglSetPixelFormatRunner below) already set one.
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    HdcVirtualizationService::Get().EnsurePixelFormatSet(nullptr);
    auto realFunction = player::g_DispatchTable.wglCreateContext.load(std::memory_order_acquire);
    command.m_Return.Value = realFunction(real_hDc);
    UpdateOutputHandle(command.m_Return);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void WglCreateContextAttribsARBRunner::Run() {
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_hShareContext);
  HDC real_hDC = HdcVirtualizationService::Get().Resolve(command.m_hDC.Value);
  // Defensive fallback for an app that never called (or that called an unsupported variant
  // of) SetPixelFormat before creating its context - see
  // docs/development/opengl2-player-skeleton-plan.md's Decision 9. A no-op if a real,
  // replayed wglSetPixelFormat command (see WglSetPixelFormatRunner below) already set one.
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    HdcVirtualizationService::Get().EnsurePixelFormatSet(nullptr);
    auto realFunction =
        player::g_DispatchTable.wglCreateContextAttribsARB.load(std::memory_order_acquire);
    // command.m_attribList.HasValue (not just .Data.data()) is the correct null check here - a
    // captured null attribList is a legal, meaningful call ("use default attributes"; the ARB
    // extension spec requires a driver to accept a null attribList the same way it requires a
    // 0-terminated one). std::vector::data() on an empty-but-HasValue-false vector is NOT
    // guaranteed to be nullptr by the standard, so forwarding it unconditionally can hand the
    // driver a non-null-but-non-dereferenceable pointer that it then scans for a 0 terminator -
    // undefined behavior/crash risk. Mirrors WglSetPixelFormatRunner::Run()'s own
    // command.m_ppfd.HasValue check below (same class of bug, already fixed there).
    command.m_Return.Value =
        realFunction(real_hDC, command.m_hShareContext.Value,
                     command.m_attribList.HasValue ? command.m_attribList.Data.data() : nullptr);
    UpdateOutputHandle(command.m_Return);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void WglMakeCurrentRunner::Run() {
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_newContext);
  // A recorded wglMakeCurrent(NULL, NULL) - the Win32 API's own "detach the current context"
  // form - has a null captured HDC. HdcVirtualizationService::Resolve() unconditionally maps
  // any recorded HDC (null or not) onto the one player-owned real HDC, so blindly resolving
  // here would turn a detach into wglMakeCurrent(real_hdc, NULL), which is not a valid detach
  // call and fails, leaving whatever context was previously current still current instead -
  // corrupting every later command that assumes the recorded detach actually happened. Pass a
  // literal null HDC straight through in that case instead of resolving it.
  HDC real_hDc =
      command.m_hDc.Value ? HdcVirtualizationService::Get().Resolve(command.m_hDc.Value) : nullptr;
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    auto realFunction = player::g_DispatchTable.wglMakeCurrent.load(std::memory_order_acquire);
    command.m_Return.Value = realFunction(real_hDc, command.m_newContext.Value);
    if (command.m_Return.Value && command.m_newContext.Value) {
      // A context just became current on this thread - the only reliable window in which
      // wglGetProcAddress can resolve extension functions (see WarmUpResolvableFunctions's own
      // comment in openglDriversAuto.h). Real apps commonly release this exact context shortly
      // after (the "dummy context -> fetch ARB functions -> destroy it -> create the real
      // context" bootstrap pattern), so anything not resolved by then would otherwise fail
      // permanently once this command's own real call resolves it lazily on first use instead.
      // Passing the context's stable, stream-decoded GITSKey (command.m_newContext.Key, untouched
      // by UpdateHandle above - that call only ever writes .Value) rather than its live,
      // replay-time HGLRC lets WarmUpResolvableFunctions() key its already-warmed cache on an
      // identity that is never reused for a different logical context - see that function's own
      // comment for why keying on the raw handle instead would have let a Windows-recycled HGLRC
      // value for a genuinely new context skip warm-up entirely.
      player::WarmUpResolvableFunctions(command.m_newContext.Key);
    }
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void GlGetUniformLocationRunner::Run() {
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_program, GLObjectKind::Program);
  const GLint capturedLocation = command.m_Return.Value;
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    auto realFunction =
        player::g_DispatchTable.glGetUniformLocation.load(std::memory_order_acquire);
    // command.m_name.HasValue check mirrors WglCreateContextAttribsARBRunner::Run()'s own
    // attribList fix above - an empty-but-HasValue-false vector's .data() is not guaranteed
    // nullptr, and a captured null `name` (an application bug, but one the recorder faithfully
    // captured rather than rejected) must replay as a real null pointer, not an
    // implementation-defined non-null-but-empty one.
    command.m_Return.Value = realFunction(
        command.m_program.Value, command.m_name.HasValue ? command.m_name.Data.data() : nullptr);
    // Keyed by the program's stream-stable GITSKey (command.m_program.Key, already resolved
    // above by UpdateHandle - which only ever writes .Value, never touches .Key), not the live,
    // replay-time GLuint - see UniformLocationMapService's own header comment for why keying by
    // the live handle collides across independent, non-sharing contexts (each share group's
    // driver-assigned names start again from small integers, so two unrelated programs can both
    // be live handle 1 at the same time).
    const GLint replayLocation = command.m_Return.Value;
    UniformLocationMapService::Get().Register(command.m_program.Key, capturedLocation,
                                              replayLocation);
    // The other elements of a uniform array this location belongs to are registered by the
    // UniformArrayMetaCommand the recorder records right after this command
    // (uniformArrayMetaRunner.h): only it knows the capture-time array size.
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void GlUseProgramRunner::Run() {
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_program, GLObjectKind::Program);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    auto realFunction = player::g_DispatchTable.glUseProgram.load(std::memory_order_acquire);
    realFunction(command.m_program.Value);
    // See UniformLocationMapService's header comment: classic glUniform*() calls carry no
    // program parameter of their own, so the "current program" must be tracked here, at the one
    // call that changes it, for the generic location-resolve hook to consult later - keyed by
    // CurrentContext(), since GL_CURRENT_PROGRAM is per-context state, not process-global.
    //
    // Only update the tracked "current program" if the real driver actually switched to the
    // requested one. glUseProgram leaves GL_CURRENT_PROGRAM unchanged if `program` fails to
    // link/validate (or any other documented failure) - blindly tracking the REQUESTED program
    // regardless of outcome would then desynchronize this service's notion of "current program"
    // from the driver's real one, corrupting later classic glUniform*() resolution. Querying the
    // real, driver-reported current program and comparing it to what was requested - rather than
    // trusting the call's success/failure return value alone - also self-corrects if some other,
    // as-yet-unmodeled path changed the binding independently.
    GLint actualProgram = 0;
    auto realGetIntegerv = player::g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire);
    realGetIntegerv(GL_CURRENT_PROGRAM, &actualProgram);
    if (static_cast<GLuint>(actualProgram) == command.m_program.Value) {
      UniformLocationMapService::Get().SetCurrentProgram(CurrentContext(), command.m_program.Key);
    }
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void WglSetPixelFormatRunner::Run() {
  auto& manager = PlayerManager::Get();
  HdcVirtualizationService::Get().Resolve(command.m_hdc.Value);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  // A captured wglSetPixelFormat(hdc, format, nullptr) is itself an invalid request - it must
  // have FAILED at capture (no format was set, no state changed there either). Preserve that
  // failure/no-op semantics on replay rather than forwarding the null straight through to
  // EnsurePixelFormatSet(), where null is ALSO the sentinel for "no real request available,
  // pick a generic fallback" (reserved for wglCreateContext(ARB)'s defensive runners - see that
  // method's own header comment). Conflating the two
  // would incorrectly set/latch a real pixel format for a call that made no state change at
  // capture, which could then block a later, legitimately-formatted wglSetPixelFormat call,
  // since Windows only allows setting a format once per HDC.
  //
  // command.m_ppfd.HasValue (not just an empty/null Data.data()) is the correct test here -
  // ArrayArgument<T>'s eager-copy constructor only sets HasValue when the real, captured
  // pointer was itself non-null (see arguments.h) - checking that flag explicitly, rather than
  // relying on std::vector::data()'s implementation-defined (never guaranteed-null) value for
  // an empty vector, is the same signal the recorder itself used to decide whether anything was
  // ever captured.
  if (manager.ExecuteCommands() && !command.m_Skip) {
    if (!command.m_ppfd.HasValue) {
      // TODO: Use FALSE, not 0, for WGL code once it's available.
      command.m_Return.Value = 0; // Mirrors the real call's own capture-time failure; still
                                  // resolves/virtualizes the HDC above so a later, real request
                                  // against the same recorded HDC value works.
    } else {
      // The recorded format index (*ipfd) is a driver/GPU-specific value with no defined meaning
      // on a possibly different replay-time driver - re-negotiate the captured
      // PIXELFORMATDESCRIPTOR request instead of replaying that index verbatim (mirrors
      // OpenGL/common/wglFunctions.cpp's CwglSetPixelFormat::Run(), which does the same
      // re-derivation for the legacy backend - see Decision 9). EnsurePixelFormatSet() itself
      // enforces "at most once", so replaying more than one recorded wglSetPixelFormat call (e.g.
      // a create-dummy-context-first bootstrap that sets a format twice on two different HDCs,
      // both virtualized onto this one real HDC) is safe.
      command.m_Return.Value =
          HdcVirtualizationService::Get().EnsurePixelFormatSet(command.m_ppfd.Data.data());
    }
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void GlViewportRunner::Run() {
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    auto realFunction = player::g_DispatchTable.glViewport.load(std::memory_order_acquire);
    realFunction(command.m_x.Value, command.m_y.Value, command.m_width.Value,
                 command.m_height.Value);
    // See HdcVirtualizationService::GrowToFit()'s own header comment for the full rationale:
    // this is a DEFENSIVE FALLBACK ONLY. This backend's single skeleton window starts at an
    // arbitrary, hardcoded 640x480 solely as Resolve()'s own fallback default - for a stream
    // with successful Win32 geometry capture (WindowGeometryTrackingService), SetGeometry()
    // normally supplies the real size before Resolve() ever creates the window, and GrowToFit()
    // then stays permanently disabled (see m_HasRealGeometry). Only when that capture never
    // happened (an older stream, or one where WindowFromDC/GetClientRect failed) does the window
    // actually start at 640x480 and need growing - GrowToFit() is called unconditionally here
    // regardless, since it already no-ops correctly once real geometry is in effect; an
    // origin-anchored (x==0, y==0) glViewport call remains the substitute signal for that
    // fallback case, same as before this capture existed.
    // Deliberately unconditional on which framebuffer is currently bound: this player does not
    // track framebuffer-binding state (a real per-context GL_DRAW_FRAMEBUFFER_BINDING query would
    // require an extra live driver call on every glViewport just to decide whether to skip this),
    // and growing the window too far for an FBO-only viewport is a harmless, purely cosmetic
    // no-op (the default framebuffer is simply larger than the app ever draws into) - unlike
    // never growing it at all, which is the actual bug this fixes.
    if (command.m_x.Value == 0 && command.m_y.Value == 0) {
      HdcVirtualizationService::Get().GrowToFit(command.m_width.Value, command.m_height.Value);
    }
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

} // namespace gits::opengl2
