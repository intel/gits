<%page args="commands, namespace='gits::opengl2', warmup_commands=None"/>\
<%
    # WarmUpResolvableFunctions() (further below) is the only per-command loop in this file that
    # can safely iterate a *narrower* list than the dispatch table/ResolveAndCall_<cmd>/
    # TryCacheRealFunction machinery above it - those three must still cover every name in
    # `commands` (the player's own hand-written code calls some disabled commands' real,
    # driver-resolved function directly through g_DispatchTable, bypassing the generated
    # per-command Run()/CommandFactory path entirely - see generator_player.py's own
    # comment for the exact names and why), so `commands` itself is never filtered here.
    # Defaults to `commands` unchanged (both for the recorder's own instantiation, which never
    # calls WarmUpResolvableFunctions() at all, and for any future caller that doesn't care to
    # narrow it).
    if warmup_commands is None:
        warmup_commands = commands
%>\
// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================
${AUTO_GENERATED_HEADER}

#include "openglDriversAuto.h"
#include "log.h"
#include "realDriver.h"

#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace ${namespace} {

// Definition of the dispatch table declared in openglDriversAuto.h.
OpenGLDispatchTable g_DispatchTable;

namespace {

// Per the WGL spec, wglGetProcAddress signals failure either by returning NULL or by
// returning one of four documented sentinel values - (void*)1, (void*)2, (void*)3, (void*)-1 -
// some drivers use instead of NULL for a function they don't support. Treating a sentinel as
// a real, resolved pointer would cache it in the dispatch table and eventually crash the
// first time something actually calls through it, since it is not a valid code address.
bool IsWglGetProcAddressFailureSentinel(void* real) {
  return real == nullptr || real == reinterpret_cast<void*>(1) ||
         real == reinterpret_cast<void*>(2) || real == reinterpret_cast<void*>(3) ||
         real == reinterpret_cast<void*>(-1);
}

// Resolves `name`'s real function pointer: tries the real opengl32.dll's
// static exports first; most functions beyond GL1.1/core WGL aren't
// statically exported at all and can only be resolved via
// wglGetProcAddress (which itself needs a context current to work
// reliably - hence this can fail and needs retrying later, done by each
// resolve-thunk below simply being called again on the next use).
// wglGetProcAddress itself is excluded from the fallback (it's always
// resolvable via the static path, being core WGL) to avoid infinite
// recursion into itself.
//
// `logFailure` distinguishes this function's two callers: ResolveAndCall_<cmd> (below) resolves
// a command that is actually about to be replayed, so a failure there means that specific call
// is about to silently do nothing - genuinely actionable, always logged. WarmUpResolvableFunctions
// (further below) instead speculatively probes every enabled command up front, the overwhelming
// majority of which are vendor-specific extensions (NV/AMD/ATI/SGIX/APPLE/INTEL/etc.) that any
// single real driver only implements a small fraction of - so almost all of its failures are
// expected, non-actionable misses that would otherwise bury a genuine ResolveAndCall_<cmd>
// warning in hundreds of harmless lines (see that function's own call site below, which passes
// logFailure=false and reports one aggregate count instead).
void* ResolveDriverFunction(const char* name, bool logFailure = true) {
  void* real = GetRealProcAddress(name);
  if (real == nullptr && strcmp(name, "wglGetProcAddress") != 0) {
    real = reinterpret_cast<void*>(
      g_DispatchTable.wglGetProcAddress.load(std::memory_order_acquire)(name));
    if (IsWglGetProcAddressFailureSentinel(real)) {
      real = nullptr;
    }
  }
  if (real == nullptr && logFailure) {
    // Failed to load it, will retry the next time app calls it.
    // Perhaps the app made a mistake of calling it before a context was available?
    // Perhaps the app did everything properly and we failed for some reason?
    // TODO: distinguish "no context current yet" from a genuine resolution
    // failure with a more specific message (see the old OpenGL backend's
    // get_proc_address(), which checks drv_.gl.Api() == CGlDriver::API_NULL
    // for this same case).
    static std::mutex warningMutex;
    static std::unordered_set<std::string> alreadyWarnedNames;
    const std::lock_guard lock(warningMutex);
    if (alreadyWarnedNames.insert(name).second) {
      LOG_WARNING << "Failed to resolve function: " << name;
    }
  }
  return real;
}

} // namespace

% for cmd in commands:
<%
    ret = cmd.return_value.type.strip()
    is_void = ret.lower() == 'void'  # WGL commands use Win32 'VOID', GL commands use 'void'.
    param_decls = ', '.join(f'{p.type} {p.name}' for p in cmd.params)
    param_names = ', '.join(p.name for p in cmd.params)
%>\
${ret} APIENTRY ResolveAndCall_${cmd.name}(${param_decls}) {
  void* real = ResolveDriverFunction("${cmd.name}");
  if (real == nullptr) {
% if is_void:
    return;
% else:
    return {};
% endif
  }
  auto realFunction = reinterpret_cast<OpenGLDispatchTable::${cmd.name}Ptr>(real);
  g_DispatchTable.${cmd.name}.store(realFunction, std::memory_order_release);
% if is_void:
  realFunction(${param_names});
% else:
  return realFunction(${param_names});
% endif
}

% endfor
bool TryCacheRealFunction(const char* name, void* value) {
  // This function is rarely run, so doesn't really matter for performance.

  static const std::unordered_map<std::string, void (*)(void*)> nameToSetter = {
% for cmd in commands:
    {"${cmd.name}", [](void* pointer) {
         g_DispatchTable.${cmd.name}.store(
           reinterpret_cast<OpenGLDispatchTable::${cmd.name}Ptr>(pointer),
           std::memory_order_release);
     }},
% endfor
  };
  auto it = nameToSetter.find(name);
  if (it == nameToSetter.end()) {
    return false;
  }
  it->second(value);
  return true;
}

void WarmUpResolvableFunctions() {
  // Best-effort, silent per-function (logFailure=false - see ResolveDriverFunction's own
  // comment on why): a name that fails here simply keeps its lazy resolve-thunk in place for a
  // later retry, exactly as if this function had never run - including still warning normally
  // through ResolveAndCall_<cmd> if that retry also fails once the app actually calls it. A name
  // that succeeds gets cached now, while it's actually resolvable, instead of on first use
  // (which may be too late - see this function's own declaration comment in
  // openglDriversAuto.h). One aggregate INFO line reports how many of the enabled commands this
  // real driver actually implements, without the per-name noise.
  //
  // Deliberately probes `warmup_commands`, not every name the dispatch table above has an
  // entry for: a disabled command never replays through the generated per-command Run() (see
  // commandRunnersAuto.cpp.mako's own `% if cmd.enabled:` guard) - it always replays through
  // the generic UnsupportedCommandRunner instead, which never touches g_DispatchTable - so
  // probing one up front is pure waste, multiplied by however many times this whole function
  // reruns per replay (every context-attaching wglMakeCurrent - see this function's own comment
  // further below). See generator_player.py's own comment for exactly which commands this
  // excludes (every disabled, mostly-vendor-extension command) and keeps (every enabled one,
  // plus a small, explicit set of disabled commands the player's own hand-written code still
  // resolves through this same dispatch table directly).
  unsigned resolvedCount = 0;
  constexpr unsigned totalCount = ${len(warmup_commands)};
% for cmd in warmup_commands:
  if (g_DispatchTable.${cmd.name}.load(std::memory_order_acquire) != &ResolveAndCall_${cmd.name}) {
    // Already resolved (and cached) by an earlier warm-up or by actual use - the dispatch-table
    // entry no longer points at its default resolve-thunk, so probing again would just repeat
    // a GetProcAddress/wglGetProcAddress call for no benefit
    // (apps that call wglMakeCurrent every frame would otherwise re-probe thousands of
    // functions per frame). Still counts toward the summary line below, so the reported
    // fraction stays accurate.
    ++resolvedCount;
  } else if (void* resolved = ResolveDriverFunction("${cmd.name}", /*logFailure=*/false)) {
    TryCacheRealFunction("${cmd.name}", resolved);
    ++resolvedCount;
  }
% endfor

  // Every successful context-attaching wglMakeCurrent call re-runs this whole function (see
  // WglMakeCurrentRunner::Run()/the recorder's own WglMakeCurrent wrapper), since a later
  // context (e.g. a real context created after a temporary bootstrap one) may resolve
  // additional functions the earlier context could not - the resolution work itself must
  // still run every time. Only the aggregate log line is deduplicated: identical back-to-back
  // counts (the overwhelmingly common case - most apps' second/third wglMakeCurrent call
  // resolves nothing new) would otherwise print the same line once per wglMakeCurrent call,
  // burying the one INFO line that actually carries information (the resolved count changing).
  static std::mutex logMutex;
  static unsigned lastLoggedCount = 0;
  static bool loggedOnce = false;
  const std::lock_guard lock(logMutex);
  if (!loggedOnce || resolvedCount != lastLoggedCount) {
    LOG_INFO << "Resolved " << resolvedCount << "/" << totalCount
             << " OpenGL/WGL functions against the real driver.";
    lastLoggedCount = resolvedCount;
    loggedOnce = true;
  }
}

} // namespace ${namespace}
