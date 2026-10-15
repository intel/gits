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
#include "log.h"
#include "openglDriversAuto.h"
#include "pbufferService.h"
#include "playerManager.h"
#include "replayContextCaps.h"
#include "shareGroupService.h"
#include "threadContextService.h"
#include "uniformLocationMapService.h"

#include <atomic>
#include <optional>
#include <vector>

namespace gits::opengl2 {

namespace {
// Zero-valued attributes are left out: wglChoosePixelFormatARB matches booleans exactly, so
// requiring e.g. WGL_BIND_TO_TEXTURE_RGB_ARB to be FALSE would reject formats that merely support
// more. WGL_DOUBLE_BUFFER_ARB and WGL_SAMPLE_BUFFERS_ARB are kept either way: the first decides
// the default draw buffer, the second whether the pbuffer is multisampled.
int ChoosePbufferPixelFormat(HDC hdc, const std::vector<int32_t>& captured, int recordedFormat) {
  std::vector<int> attributes;
  for (size_t i = 0; i + 1 < captured.size(); i += 2) {
    if (captured[i + 1] != 0 || captured[i] == WGL_DOUBLE_BUFFER_ARB ||
        captured[i] == WGL_SAMPLE_BUFFERS_ARB) {
      attributes.push_back(captured[i]);
      attributes.push_back(captured[i + 1]);
    }
  }
  if (!attributes.empty()) {
    attributes.push_back(0);
    int format = 0;
    UINT count = 0;
    auto choose = player::g_DispatchTable.wglChoosePixelFormatARB.load(std::memory_order_acquire);
    if (choose(hdc, attributes.data(), nullptr, 1, &format, &count) && count > 0) {
      return format;
    }
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
      PbufferService::Get().TakePendingPixelFormat(command.m_ThreadId);
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

// Core and GL_ARB_shader_objects uniform location commands differ only in their program
// parameter's name and entry points.
template <typename CommandT, typename RealFunctionT>
void RunGetUniformLocation(CommandT& command,
                           HandleArgument<GLuint>& program,
                           const std::atomic<RealFunctionT>& realFunction,
                           const char* commandName) {
  auto& manager = PlayerManager::Get();
  if (!UpdateHandle(program, GLObjectKind::Program)) {
    SkipUnmappedObjectNameCall(command, commandName);
  }
  const GLint capturedLocation = command.m_Return.Value;
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    // command.m_name.HasValue check mirrors WglCreateContextAttribsARBRunner::Run()'s own
    // attribList fix - an empty-but-HasValue-false vector's .data() is not guaranteed
    // nullptr, and a captured null `name` (an application bug, but one the recorder faithfully
    // captured rather than rejected) must replay as a real null pointer, not an
    // implementation-defined non-null-but-empty one.
    command.m_Return.Value = realFunction.load(std::memory_order_acquire)(
        program.Value, command.m_name.HasValue ? command.m_name.Data.data() : nullptr);
    // Keyed by the program's stream-stable GITSKey (program.Key, already resolved above by
    // UpdateHandle - which only ever writes .Value, never touches .Key), not the live,
    // replay-time GLuint - see UniformLocationMapService's own header comment for why keying by
    // the live handle collides across independent, non-sharing contexts (each share group's
    // driver-assigned names start again from small integers, so two unrelated programs can both
    // be live handle 1 at the same time).
    const GLint replayLocation = command.m_Return.Value;
    UniformLocationMapService::Get().Register(program.Key, capturedLocation, replayLocation);
    // The other elements of a uniform array this location belongs to are registered by the
    // UniformArrayMetaCommand the recorder records right after this command
    // (uniformArrayMetaRunner.h): only it knows the capture-time array size.
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

template <typename CommandT, typename RealFunctionT, typename QueryCurrentProgramT>
void RunUseProgram(CommandT& command,
                   HandleArgument<GLuint>& program,
                   const std::atomic<RealFunctionT>& realFunction,
                   const char* commandName,
                   QueryCurrentProgramT queryCurrentProgram) {
  auto& manager = PlayerManager::Get();
  if (!UpdateHandle(program, GLObjectKind::Program)) {
    SkipUnmappedObjectNameCall(command, commandName);
  }
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    realFunction.load(std::memory_order_acquire)(program.Value);
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
    if (queryCurrentProgram() == program.Value) {
      UniformLocationMapService::Get().SetCurrentProgram(CurrentContext(), program.Key);
    }
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

// As RunUseProgram, the binding is tracked only once GL reports it.
template <typename CommandT, typename RealFunctionT>
void RunBindProgramPipeline(CommandT& command,
                            const std::atomic<RealFunctionT>& realFunction,
                            const char* commandName) {
  auto& manager = PlayerManager::Get();
  if (!UpdateHandle(command.m_pipeline, GLObjectKind::ProgramPipeline)) {
    SkipUnmappedObjectNameCall(command, commandName);
  }
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    realFunction.load(std::memory_order_acquire)(command.m_pipeline.Value);
    GLint pipeline = 0;
    player::g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire)(
        GL_PROGRAM_PIPELINE_BINDING, &pipeline);
    if (static_cast<GLuint>(pipeline) == command.m_pipeline.Value) {
      UniformLocationMapService::Get().BindProgramPipeline(CurrentContext(),
                                                           command.m_pipeline.Key);
    }
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

// GL keeps the previous active program if `program` is not linked, so it is queried back.
template <typename CommandT, typename RealFunctionT, typename QueryFunctionT>
void RunActiveShaderProgram(CommandT& command,
                            const std::atomic<RealFunctionT>& realFunction,
                            const std::atomic<QueryFunctionT>& queryFunction,
                            const char* commandName) {
  auto& manager = PlayerManager::Get();
  if (!UpdateHandle(command.m_pipeline, GLObjectKind::ProgramPipeline)) {
    SkipUnmappedObjectNameCall(command, commandName);
  }
  if (!UpdateHandle(command.m_program, GLObjectKind::Program)) {
    SkipUnmappedObjectNameCall(command, commandName);
  }
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    realFunction.load(std::memory_order_acquire)(command.m_pipeline.Value, command.m_program.Value);
    GLint program = 0;
    queryFunction.load(std::memory_order_acquire)(command.m_pipeline.Value, GL_ACTIVE_PROGRAM,
                                                  &program);
    if (static_cast<GLuint>(program) == command.m_program.Value) {
      UniformLocationMapService::Get().SetActiveProgram(command.m_pipeline.Key,
                                                        command.m_program.Key);
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

void GlGetUniformLocationRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  RunGetUniformLocation(command, command.m_program, player::g_DispatchTable.glGetUniformLocation,
                        "glGetUniformLocation");
}

void GlGetUniformLocationARBRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  RunGetUniformLocation(command, command.m_programObj,
                        player::g_DispatchTable.glGetUniformLocationARB, "glGetUniformLocationARB");
}

void GlUseProgramRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  RunUseProgram(command, command.m_program, player::g_DispatchTable.glUseProgram, "glUseProgram",
                [] {
                  GLint program = 0;
                  player::g_DispatchTable.glGetIntegerv.load(std::memory_order_acquire)(
                      GL_CURRENT_PROGRAM, &program);
                  return static_cast<GLuint>(program);
                });
}

void GlUseProgramObjectARBRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  // GL_CURRENT_PROGRAM is a GL 2.0 query; a GL_ARB_shader_objects-only driver would reject it.
  RunUseProgram(command, command.m_programObj, player::g_DispatchTable.glUseProgramObjectARB,
                "glUseProgramObjectARB", [] {
                  return player::g_DispatchTable.glGetHandleARB.load(std::memory_order_acquire)(
                      GL_PROGRAM_OBJECT_ARB);
                });
}

void GlBindProgramPipelineRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  RunBindProgramPipeline(command, player::g_DispatchTable.glBindProgramPipeline,
                         "glBindProgramPipeline");
}

void GlBindProgramPipelineEXTRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  RunBindProgramPipeline(command, player::g_DispatchTable.glBindProgramPipelineEXT,
                         "glBindProgramPipelineEXT");
}

void GlActiveShaderProgramRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  RunActiveShaderProgram(command, player::g_DispatchTable.glActiveShaderProgram,
                         player::g_DispatchTable.glGetProgramPipelineiv, "glActiveShaderProgram");
}

void GlActiveShaderProgramEXTRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  RunActiveShaderProgram(command, player::g_DispatchTable.glActiveShaderProgramEXT,
                         player::g_DispatchTable.glGetProgramPipelineivEXT,
                         "glActiveShaderProgramEXT");
}

void WglSetPixelFormatRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  HdcVirtualizationService::Get().BindSampledWindow(command.m_hdc.Value, true);
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
      // enforces "at most once" per player window.
      command.m_Return.Value = HdcVirtualizationService::Get().EnsurePixelFormatSet(
          command.m_hdc.Value, command.m_ppfd.Data.data());
    }
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

void GlViewportRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    auto realFunction = player::g_DispatchTable.glViewport.load(std::memory_order_acquire);
    realFunction(command.m_x.Value, command.m_y.Value, command.m_width.Value,
                 command.m_height.Value);
    // See HdcVirtualizationService::GrowToFit()'s own header comment for the full rationale:
    // this is a DEFENSIVE FALLBACK ONLY. It sizes only the fallback window, which starts at an
    // arbitrary, hardcoded 640x480 and is used only when no recorded window is known (an older
    // stream, or one where WindowFromDC/GetClientRect failed); windows identified by
    // WindowGeometryTrackingService samples take their real size from those samples.
    // GrowToFit() is called unconditionally here regardless, since it no-ops unless the fallback
    // window is the current draw window; an origin-anchored (x==0, y==0) glViewport call remains
    // the substitute signal for that fallback case, same as before this capture existed.
    // Deliberately unconditional on which framebuffer is currently bound: this player does not
    // track framebuffer-binding state (a real per-context GL_DRAW_FRAMEBUFFER_BINDING query would
    // require an extra live driver call on every glViewport just to decide whether to skip this),
    // and growing the window too far for an FBO-only viewport is a harmless, purely cosmetic
    // no-op (the default framebuffer is simply larger than the app ever draws into) - unlike
    // never growing it at all, which is the actual bug this fixes.
    if (command.m_x.Value == 0 && command.m_y.Value == 0) {
      HdcVirtualizationService::Get().GrowToFit(
          player::g_DispatchTable.wglGetCurrentDC.load(std::memory_order_acquire)(),
          command.m_width.Value, command.m_height.Value);
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

void GlClientWaitSyncRunner::Run() {
  ThreadContextService::Get().SwitchTo(command.m_ThreadId);
  auto& manager = PlayerManager::Get();
  UpdateHandle(command.m_sync);
  for (Layer* layer : manager.GetPreLayers()) {
    layer->Pre(command);
  }
  if (manager.ExecuteCommands() && !command.m_Skip) {
    // The application went on as if the GPU had reached the sync object, so its next commands
    // may overwrite data the GPU is still reading; a slower replay GPU must catch up first. The
    // retries flush, so they cannot wait on commands the driver has not submitted.
    constexpr GLuint64 retryTimeoutNs = 1000000000;
    const GLenum recorded = command.m_Return.Value;
    auto clientWaitSync = player::g_DispatchTable.glClientWaitSync.load(std::memory_order_acquire);
    GLenum result =
        clientWaitSync(command.m_sync.Value, command.m_flags.Value, command.m_timeout.Value);
    if (recorded == GL_ALREADY_SIGNALED || recorded == GL_CONDITION_SATISFIED) {
      while (result == GL_TIMEOUT_EXPIRED) {
        result = clientWaitSync(command.m_sync.Value, GL_SYNC_FLUSH_COMMANDS_BIT, retryTimeoutNs);
      }
    }
    command.m_Return.Value = result;
  }
  for (Layer* layer : manager.GetPostLayers()) {
    layer->Post(command);
  }
}

} // namespace gits::opengl2
