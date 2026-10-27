// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "commandRunnersCustom.h"
#include "glEnumsAuto.h"
#include "glxDrawableService.h"
#include "glxFBConfigService.h"
#include "handleArgumentUpdaters.h"
#include "hdcVirtualizationService.h"
#include "layerAuto.h"
#include "log.h"
#include "openglDriversAuto.h"
#include "pbufferService.h"
#include "playerManager.h"
#include "replayContextCaps.h"
#include "shareGroupService.h"
#include "threadContextService.h"

#include <atomic>
#include <optional>
#include <vector>

namespace gits::opengl2 {

namespace {
int ChoosePbufferPixelFormat(HDC hdc, const std::vector<int32_t>& captured, int recordedFormat) {
  const int format = HdcVirtualizationService::ChooseCapturedPixelFormat(hdc, captured);
  if (format != 0) {
    return format;
  }
  static std::atomic<bool> warned{false};
  if (!warned.exchange(true)) {
    LOG_WARNING << "wglCreatePbufferARB/EXT: no replay pixel format matches the captured one - "
                   "using the captured format index";
  }
  return recordedFormat;
}

// WGL_ARB_pbuffer and WGL_EXT_pbuffer differ only in their handle type and entry points.
template <typename CommandT, typename RealFunctionT>
void RunCreatePbuffer(CommandT& command, const std::atomic<RealFunctionT>& realFunction) {
  auto& manager = PlayerManager::Get();
  HDC real_hDC = HdcVirtualizationService::Get().Resolve(command.m_hDC.Value);
  const std::vector<int32_t> pixelFormat =
      HdcVirtualizationService::Get().TakePendingPixelFormat(command.m_ThreadId);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    command.m_Return.Value = realFunction.load(std::memory_order_acquire)(
        real_hDC, ChoosePbufferPixelFormat(real_hDC, pixelFormat, command.m_iPixelFormat.Value),
        command.m_iWidth.Value, command.m_iHeight.Value,
        command.m_piAttribList.HasValue ? command.m_piAttribList.Data.data() : nullptr);
    UpdateOutputHandle(command.m_Return);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

template <typename CommandT, typename RealFunctionT>
void RunGetPbufferDC(CommandT& command, const std::atomic<RealFunctionT>& realFunction) {
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_hPbuffer);
  const HDC recordedDc = command.m_Return.Value;
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    // The return keeps the recorded DC, which later commands name and Resolve() maps.
    const HDC realDc = realFunction.load(std::memory_order_acquire)(command.m_hPbuffer.Value);
    if (recordedDc != nullptr) {
      PbufferService::Get().AcquireDc(recordedDc, command.m_hPbuffer.Key, realDc);
      static std::atomic<bool> warned{false};
      if (realDc == nullptr && !warned.exchange(true)) {
        LOG_WARNING << "wglGetPbufferDCARB/EXT: replay failed to get a pbuffer DC capture got - "
                       "calls using it fail";
      }
    }
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

template <typename CommandT, typename RealFunctionT>
void RunReleasePbufferDC(CommandT& command, const std::atomic<RealFunctionT>& realFunction) {
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_hPbuffer);
  const HDC recordedDc = command.m_hDC.Value;
  const std::optional<HDC> real_hDC = PbufferService::Get().FindDc(recordedDc);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip && real_hDC.has_value()) {
    // A DC replay failed to get has nothing to release; the call fails like a release of an
    // invalid DC. A failed release of a real DC leaves it acquired.
    command.m_Return.Value =
        *real_hDC == nullptr
            ? 0
            : realFunction.load(std::memory_order_acquire)(command.m_hPbuffer.Value, *real_hDC);
    if (*real_hDC == nullptr || command.m_Return.Value != 0) {
      PbufferService::Get().ReleaseDc(recordedDc);
    }
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

template <typename CommandT, typename RealFunctionT>
void RunDestroyPbuffer(CommandT& command, const std::atomic<RealFunctionT>& realFunction) {
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_hPbuffer);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    command.m_Return.Value = realFunction.load(std::memory_order_acquire)(command.m_hPbuffer.Value);
    if (command.m_Return.Value) {
      PbufferService::Get().ErasePbuffer(command.m_hPbuffer.Key);
    }
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

} // namespace

void WglCreateContextRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  HdcVirtualizationService::Get().BindSampledWindow(command.m_hDc.Value);
  HDC real_hDc = HdcVirtualizationService::Get().Resolve(command.m_hDc.Value);
  // Defensive fallback for an app that never called (or that called an unsupported variant
  // of) SetPixelFormat before creating its context - see
  // docs/development/opengl2-player-skeleton-plan.md's Decision 9. A no-op if a real,
  // replayed wglSetPixelFormat command (see WglSetPixelFormatRunner below) already set one.
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    HdcVirtualizationService::Get().EnsurePixelFormatSet(command.m_hDc.Value, nullptr);
    auto realFunction = player::g_DispatchTable.wglCreateContext.load(std::memory_order_acquire);
    command.m_Return.Value = realFunction(real_hDc);
    UpdateOutputHandle(command.m_Return);
    ShareGroupService::Get().RegisterNewContext(command.m_Return.Value, nullptr);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void WglCreateContextAttribsARBRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_hShareContext);
  HdcVirtualizationService::Get().BindSampledWindow(command.m_hDC.Value);
  HDC real_hDC = HdcVirtualizationService::Get().Resolve(command.m_hDC.Value);
  // Defensive fallback for an app that never called (or that called an unsupported variant
  // of) SetPixelFormat before creating its context - see
  // docs/development/opengl2-player-skeleton-plan.md's Decision 9. A no-op if a real,
  // replayed wglSetPixelFormat command (see WglSetPixelFormatRunner below) already set one.
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    HdcVirtualizationService::Get().EnsurePixelFormatSet(command.m_hDC.Value, nullptr);
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
    ShareGroupService::Get().RegisterNewContext(command.m_Return.Value,
                                                command.m_hShareContext.Value);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void WglMakeCurrentRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_newContext);
  HdcVirtualizationService::Get().BindSampledWindow(command.m_hDc.Value);
  // A recorded wglMakeCurrent(NULL, NULL) - the Win32 API's own "detach the current context"
  // form - has a null captured HDC. HdcVirtualizationService::Resolve() maps any recorded HDC
  // (null or not) onto a player window's real HDC, so blindly resolving here would turn a detach
  // into wglMakeCurrent(real_hdc, NULL), which is not a valid detach call and fails, leaving
  // whatever context was previously current still current instead - corrupting every later
  // command that assumes the recorded detach actually happened. Pass a literal null HDC straight
  // through in that case instead of resolving it.
  HDC real_hDc =
      command.m_hDc.Value ? HdcVirtualizationService::Get().Resolve(command.m_hDc.Value) : nullptr;
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    auto realFunction = player::g_DispatchTable.wglMakeCurrent.load(std::memory_order_acquire);
    command.m_Return.Value = realFunction(real_hDc, command.m_newContext.Value);
    if (command.m_Return.Value) {
      InvalidateReplayContextCaps();
    }
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

namespace {
template <typename MakeContextCurrentCommand, typename RealFunction>
void RunMakeContextCurrent(MakeContextCurrentCommand& command, RealFunction& realFunction) {
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_hglrc);
  auto& hdcService = HdcVirtualizationService::Get();
  hdcService.BindSampledWindows(command.m_hDrawDC.Value, command.m_hReadDC.Value);
  HDC real_hDrawDC = hdcService.Resolve(command.m_hDrawDC.Value);
  HDC real_hReadDC = hdcService.Resolve(command.m_hReadDC.Value);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    command.m_Return.Value = realFunction.load(std::memory_order_acquire)(
        real_hDrawDC, real_hReadDC, command.m_hglrc.Value);
    if (command.m_Return.Value) {
      InvalidateReplayContextCaps();
    }
    // See WglMakeCurrentRunner::Run().
    if (command.m_Return.Value && command.m_hglrc.Value) {
      player::WarmUpResolvableFunctions(command.m_hglrc.Key);
    }
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}
} // namespace

void WglMakeContextCurrentARBRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  RunMakeContextCurrent(command, player::g_DispatchTable.wglMakeContextCurrentARB);
}

void WglMakeContextCurrentEXTRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  RunMakeContextCurrent(command, player::g_DispatchTable.wglMakeContextCurrentEXT);
}

void WglSetPixelFormatRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  HdcVirtualizationService::Get().BindSampledWindow(command.m_hdc.Value, true);
  HdcVirtualizationService::Get().Resolve(command.m_hdc.Value);
  const std::vector<int32_t> pixelFormat =
      HdcVirtualizationService::Get().TakePendingPixelFormat(command.m_ThreadId);
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
      // on a possibly different replay-time driver - re-negotiate the captured attributes and
      // PIXELFORMATDESCRIPTOR instead of replaying that index verbatim (mirrors
      // OpenGL/common/wglFunctions.cpp's CwglSetPixelFormat::Run(), which does the same
      // re-derivation for the legacy backend - see Decision 9). EnsurePixelFormatSet() itself
      // enforces "at most once" per player window.
      command.m_Return.Value = HdcVirtualizationService::Get().EnsurePixelFormatSet(
          command.m_hdc.Value, command.m_ppfd.Data.data(), pixelFormat);
    }
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void WglSwapBuffersRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  HdcVirtualizationService::Get().BindSampledWindow(command.m_hdc.Value);
  HDC real_hdc = HdcVirtualizationService::Get().Resolve(command.m_hdc.Value);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    auto realFunction = player::g_DispatchTable.wglSwapBuffers.load(std::memory_order_acquire);
    command.m_Return.Value = realFunction(real_hdc);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void WglSwapLayerBuffersRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  HdcVirtualizationService::Get().BindSampledWindow(command.m_hdc.Value);
  HDC real_hdc = HdcVirtualizationService::Get().Resolve(command.m_hdc.Value);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    auto realFunction = player::g_DispatchTable.wglSwapLayerBuffers.load(std::memory_order_acquire);
    command.m_Return.Value = realFunction(real_hdc, command.m_fuFlags.Value);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void WglCreatePbufferARBRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  RunCreatePbuffer(command, player::g_DispatchTable.wglCreatePbufferARB);
}

void WglGetPbufferDCARBRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  RunGetPbufferDC(command, player::g_DispatchTable.wglGetPbufferDCARB);
}

void WglReleasePbufferDCARBRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  RunReleasePbufferDC(command, player::g_DispatchTable.wglReleasePbufferDCARB);
}

void WglDestroyPbufferARBRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  RunDestroyPbuffer(command, player::g_DispatchTable.wglDestroyPbufferARB);
}

void WglCreatePbufferEXTRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  RunCreatePbuffer(command, player::g_DispatchTable.wglCreatePbufferEXT);
}

void WglGetPbufferDCEXTRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  RunGetPbufferDC(command, player::g_DispatchTable.wglGetPbufferDCEXT);
}

void WglReleasePbufferDCEXTRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  RunReleasePbufferDC(command, player::g_DispatchTable.wglReleasePbufferDCEXT);
}

void WglDestroyPbufferEXTRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  RunDestroyPbuffer(command, player::g_DispatchTable.wglDestroyPbufferEXT);
}

#if !defined(_WIN32)
void GlxCreateContextRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_shareList);
  const GLXFBConfig config = GlxFBConfigService::Get().TakeChosen(command.m_ThreadId);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    Display* display = GlxDrawableService::Get().GetDisplay();
    auto getVisual =
        player::g_DispatchTable.glXGetVisualFromFBConfig.load(std::memory_order_acquire);
    XVisualInfo* visual = config != nullptr ? getVisual(display, config) : nullptr;
    auto realFunction = player::g_DispatchTable.glXCreateContext.load(std::memory_order_acquire);
    command.m_Return.Value =
        visual != nullptr
            ? realFunction(display, visual, command.m_shareList.Value, command.m_direct.Value)
            : nullptr;
    GlxDrawableService::Free(visual);
    UpdateOutputHandle(command.m_Return);
    ShareGroupService::Get().RegisterNewContext(command.m_Return.Value, command.m_shareList.Value);
    GlxFBConfigService::Get().SetContextVisual(command.m_Return.Value,
                                               GlxFBConfigService::VisualId(config));
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void GlxMakeCurrentRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_ctx);
  const GLXDrawable real_drawable = GlxDrawableService::Get().Resolve(
      command.m_drawable.Value, GlxFBConfigService::Get().ContextVisual(command.m_ctx.Value));
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    auto realFunction = player::g_DispatchTable.glXMakeCurrent.load(std::memory_order_acquire);
    command.m_Return.Value =
        realFunction(GlxDrawableService::Get().GetDisplay(), real_drawable, command.m_ctx.Value);
    if (command.m_Return.Value != 0) {
      InvalidateReplayContextCaps();
    }
    // See WglMakeCurrentRunner::Run().
    if (command.m_Return.Value != 0 && command.m_ctx.Value != nullptr) {
      player::WarmUpResolvableFunctions(command.m_ctx.Key);
    }
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void GlxCreateNewContextRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_share_list);
  const GLXFBConfig config = GlxFBConfigService::Get().TakeChosen(command.m_ThreadId);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    auto realFunction = player::g_DispatchTable.glXCreateNewContext.load(std::memory_order_acquire);
    command.m_Return.Value =
        realFunction(GlxDrawableService::Get().GetDisplay(), config, command.m_render_type.Value,
                     command.m_share_list.Value, command.m_direct.Value);
    UpdateOutputHandle(command.m_Return);
    ShareGroupService::Get().RegisterNewContext(command.m_Return.Value, command.m_share_list.Value);
    GlxFBConfigService::Get().SetContextVisual(command.m_Return.Value,
                                               GlxFBConfigService::VisualId(config));
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void GlxCreateContextAttribsARBRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_share_context);
  const GLXFBConfig config = GlxFBConfigService::Get().TakeChosen(command.m_ThreadId);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    auto realFunction =
        player::g_DispatchTable.glXCreateContextAttribsARB.load(std::memory_order_acquire);
    command.m_Return.Value =
        realFunction(GlxDrawableService::Get().GetDisplay(), config, command.m_share_context.Value,
                     command.m_direct.Value,
                     command.m_attrib_list.HasValue ? command.m_attrib_list.Data.data() : nullptr);
    UpdateOutputHandle(command.m_Return);
    ShareGroupService::Get().RegisterNewContext(command.m_Return.Value,
                                                command.m_share_context.Value);
    GlxFBConfigService::Get().SetContextVisual(command.m_Return.Value,
                                               GlxFBConfigService::VisualId(config));
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void GlxMakeContextCurrentRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_ctx);
  auto& drawables = GlxDrawableService::Get();
  const uint64_t visual = GlxFBConfigService::Get().ContextVisual(command.m_ctx.Value);
  const GLXDrawable real_draw = drawables.Resolve(command.m_draw.Value, visual);
  const GLXDrawable real_read = drawables.Resolve(command.m_read.Value, visual);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    auto realFunction =
        player::g_DispatchTable.glXMakeContextCurrent.load(std::memory_order_acquire);
    command.m_Return.Value =
        realFunction(drawables.GetDisplay(), real_draw, real_read, command.m_ctx.Value);
    if (command.m_Return.Value != 0) {
      InvalidateReplayContextCaps();
    }
    if (command.m_Return.Value != 0 && command.m_ctx.Value != nullptr) {
      player::WarmUpResolvableFunctions(command.m_ctx.Key);
    }
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void GlxMakeCurrentReadSGIRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_ctx);
  auto& drawables = GlxDrawableService::Get();
  const uint64_t visual = GlxFBConfigService::Get().ContextVisual(command.m_ctx.Value);
  const GLXDrawable real_draw = drawables.Resolve(command.m_draw.Value, visual);
  const GLXDrawable real_read = drawables.Resolve(command.m_read.Value, visual);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    auto realFunction =
        player::g_DispatchTable.glXMakeCurrentReadSGI.load(std::memory_order_acquire);
    command.m_Return.Value =
        realFunction(drawables.GetDisplay(), real_draw, real_read, command.m_ctx.Value);
    if (command.m_Return.Value != 0) {
      InvalidateReplayContextCaps();
    }
    if (command.m_Return.Value != 0 && command.m_ctx.Value != nullptr) {
      player::WarmUpResolvableFunctions(command.m_ctx.Key);
    }
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void GlxDestroyContextRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_ctx);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    ThreadContextService::Get().Destroy(command.m_ctx.Value);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void GlxCreateWindowRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  auto& drawables = GlxDrawableService::Get();
  const GLXFBConfig config = GlxFBConfigService::Get().TakeChosen(command.m_ThreadId);
  const Window real_win =
      drawables.Resolve(command.m_win.Value, GlxFBConfigService::VisualId(config));
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    auto realFunction = player::g_DispatchTable.glXCreateWindow.load(std::memory_order_acquire);
    const GLXWindow live =
        realFunction(drawables.GetDisplay(), config, real_win,
                     command.m_attrib_list.HasValue ? command.m_attrib_list.Data.data() : nullptr);
    if (live != 0) {
      drawables.SetSubstitute(command.m_Return.Value, live, command.m_win.Value);
    }
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void GlxDestroyWindowRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  auto& drawables = GlxDrawableService::Get();
  const GLXWindow real_win = drawables.Resolve(command.m_win.Value);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    const uint64_t recorded = command.m_win.Value;
    ThreadContextService::Get().DestroyDrawable(real_win, [&drawables, real_win, recorded] {
      auto realFunction = player::g_DispatchTable.glXDestroyWindow.load(std::memory_order_acquire);
      realFunction(drawables.GetDisplay(), real_win);
      drawables.RemoveSubstitute(recorded);
    });
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void GlxCreatePbufferRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  auto& drawables = GlxDrawableService::Get();
  const GLXFBConfig config = GlxFBConfigService::Get().TakeChosen(command.m_ThreadId);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    auto realFunction = player::g_DispatchTable.glXCreatePbuffer.load(std::memory_order_acquire);
    const GLXPbuffer live =
        realFunction(drawables.GetDisplay(), config,
                     command.m_attrib_list.HasValue ? command.m_attrib_list.Data.data() : nullptr);
    if (live != 0) {
      drawables.SetSubstitute(command.m_Return.Value, live, 0);
    }
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void GlxDestroyPbufferRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  auto& drawables = GlxDrawableService::Get();
  const GLXPbuffer real_pbuf = drawables.Resolve(command.m_pbuf.Value);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    const uint64_t recorded = command.m_pbuf.Value;
    ThreadContextService::Get().DestroyDrawable(real_pbuf, [&drawables, real_pbuf, recorded] {
      auto realFunction = player::g_DispatchTable.glXDestroyPbuffer.load(std::memory_order_acquire);
      realFunction(drawables.GetDisplay(), real_pbuf);
      drawables.RemoveSubstitute(recorded);
    });
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void GlxCreatePixmapRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  auto& drawables = GlxDrawableService::Get();
  const GLXFBConfig config = GlxFBConfigService::Get().TakeChosen(command.m_ThreadId, true);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    Display* display = drawables.GetDisplay();
    auto getVisual =
        player::g_DispatchTable.glXGetVisualFromFBConfig.load(std::memory_order_acquire);
    XVisualInfo* visual = config != nullptr ? getVisual(display, config) : nullptr;
    const Pixmap pixmap = drawables.CreatePixmap(command.m_pixmap.Value, visual);
    GlxDrawableService::Free(visual);
    if (pixmap != 0) {
      auto realFunction = player::g_DispatchTable.glXCreatePixmap.load(std::memory_order_acquire);
      const GLXPixmap live = realFunction(
          display, config, pixmap,
          command.m_attrib_list.HasValue ? command.m_attrib_list.Data.data() : nullptr);
      drawables.SetPixmapSubstitute(command.m_Return.Value, live, pixmap);
    }
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void GlxDestroyPixmapRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  auto& drawables = GlxDrawableService::Get();
  const GLXPixmap real_pixmap = drawables.Resolve(command.m_pixmap.Value);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    const uint64_t recorded = command.m_pixmap.Value;
    ThreadContextService::Get().DestroyDrawable(real_pixmap, [&drawables, real_pixmap, recorded] {
      auto realFunction = player::g_DispatchTable.glXDestroyPixmap.load(std::memory_order_acquire);
      realFunction(drawables.GetDisplay(), real_pixmap);
      drawables.RemoveSubstitute(recorded);
    });
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void GlxCreateGLXPixmapRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  auto& drawables = GlxDrawableService::Get();
  const GLXFBConfig config = GlxFBConfigService::Get().TakeChosen(command.m_ThreadId, true);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    Display* display = drawables.GetDisplay();
    auto getVisual =
        player::g_DispatchTable.glXGetVisualFromFBConfig.load(std::memory_order_acquire);
    XVisualInfo* visual = config != nullptr ? getVisual(display, config) : nullptr;
    const Pixmap pixmap =
        visual != nullptr ? drawables.CreatePixmap(command.m_pixmap.Value, visual) : 0;
    if (pixmap != 0) {
      auto realFunction =
          player::g_DispatchTable.glXCreateGLXPixmap.load(std::memory_order_acquire);
      const GLXPixmap live = realFunction(display, visual, pixmap);
      drawables.SetPixmapSubstitute(command.m_Return.Value, live, pixmap);
    }
    GlxDrawableService::Free(visual);
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void GlxDestroyGLXPixmapRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  auto& drawables = GlxDrawableService::Get();
  const GLXPixmap real_pixmap = drawables.Resolve(command.m_pixmap.Value);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    const uint64_t recorded = command.m_pixmap.Value;
    ThreadContextService::Get().DestroyDrawable(real_pixmap, [&drawables, real_pixmap, recorded] {
      auto realFunction =
          player::g_DispatchTable.glXDestroyGLXPixmap.load(std::memory_order_acquire);
      realFunction(drawables.GetDisplay(), real_pixmap);
      drawables.RemoveSubstitute(recorded);
    });
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}
#endif

} // namespace gits::opengl2
