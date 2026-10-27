<%page args="commands, namespace='gits::opengl', warmup_commands=None, platform_guards=None"/>\
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
    if platform_guards is None:
        platform_guards = {}
    # Window-system commands are probed only on their own platform.
    unguarded_warmup_count = sum(1 for c in warmup_commands if c.api not in platform_guards)
    guarded_warmup_counts = {
        guard: sum(1 for c in warmup_commands if platform_guards.get(c.api) == guard)
        for guard in platform_guards.values()
    }
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
#include <string_view>
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
// recursion into itself. Elsewhere the fallback is glXGetProcAddressARB,
// which libGL exports.
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
#if defined(_WIN32)
  if (real == nullptr && strcmp(name, "wglGetProcAddress") != 0) {
    real = reinterpret_cast<void*>(
      g_DispatchTable.wglGetProcAddress.load(std::memory_order_acquire)(name));
    if (IsWglGetProcAddressFailureSentinel(real)) {
      real = nullptr;
    }
  }
#else
  if (real == nullptr && strcmp(name, "glXGetProcAddressARB") != 0) {
    real = reinterpret_cast<void*>(g_DispatchTable.glXGetProcAddressARB.load(
      std::memory_order_acquire)(reinterpret_cast<const GLubyte*>(name)));
  }
#endif
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
namespace {
struct DispatchEntry {
  const char* Name;
  void (*Setter)(void*);
  bool (*IsResolved)();
};

const DispatchEntry* FindDispatchEntry(const char* name) {
  // A braced map initializer would construct every key inline, which makes this function very
  // slow to compile.
  static const DispatchEntry entries[] = {
% for cmd in commands:
    {"${cmd.name}", [](void* pointer) {
         g_DispatchTable.${cmd.name}.store(
           reinterpret_cast<OpenGLDispatchTable::${cmd.name}Ptr>(pointer),
           std::memory_order_release);
     }, [] {
         const auto pointer = g_DispatchTable.${cmd.name}.load(std::memory_order_acquire);
         return pointer != nullptr && pointer != &ResolveAndCall_${cmd.name};
     }},
% endfor
  };
  static const std::unordered_map<std::string_view, const DispatchEntry*> nameToEntry = [] {
    std::unordered_map<std::string_view, const DispatchEntry*> map;
    map.reserve(std::size(entries));
    for (const DispatchEntry& entry : entries) {
      map.emplace(entry.Name, &entry);
    }
    return map;
  }();
  const auto it = nameToEntry.find(name);
  return it != nameToEntry.end() ? it->second : nullptr;
}
} // namespace

bool TryCacheRealFunction(const char* name, void* value) {
  const auto* entry = FindDispatchEntry(name);
  if (entry == nullptr) {
    return false;
  }
  entry->Setter(value);
  return true;
}

bool TryResolveRealFunction(const char* name) {
  const auto* entry = FindDispatchEntry(name);
  if (entry == nullptr) {
    return false;
  }
  if (entry->IsResolved()) {
    return true;
  }
  void* real = ResolveDriverFunction(name, /*logFailure=*/false);
  if (real == nullptr) {
    return false;
  }
  entry->Setter(real);
  return true;
}

void* GetRealFunction(const char* name) {
  return ResolveDriverFunction(name, /*logFailure=*/false);
}

void WarmUpResolvableFunctions(GITSKey contextKey) {
  // Skip the entire pass below for a context this function has already warmed up - see this
  // function's own declaration comment in openglDriversAuto.h. Keyed by the context's stable
  // GITSKey, not its live HGLRC, and permanently (never invalidated on wglDeleteContext): the
  // resolvability of an extension function pointer depends on the real driver/GPU/pixel format,
  // not on which still-live context happens to be current when wglGetProcAddress is called, so
  // re-warming the SAME context later would always rediscover exactly the same set - there is
  // nothing to invalidate in the first place for a still-live context.
  //
  // GITSKey rather than HGLRC specifically because a GITSKey is minted exactly once per logical
  // context (ContextHandleMapService::Register(), called only from wglCreateContext(ARB)'s own
  // UpdateOutputHandle) and never reused, while Windows CAN and does reuse a destroyed context's
  // numeric HGLRC value for a later, entirely unrelated wglCreateContext[ARB] call. Keying on
  // the raw handle would then incorrectly treat that new, genuinely different context as
  // already warmed and skip it outright - not a harmless miss, since the whole reason this
  // function exists is to resolve functions while some OTHER context is current for later use
  // (the "dummy context -> fetch ARB functions -> destroy it -> create the real context"
  // bootstrap pattern - see this function's own declaration comment): a skipped warm-up for the
  // real, reused-handle context could leave it unable to resolve a function it needs at the
  // exact point actual use requires it, i.e. the original bug this mechanism was built to
  // prevent, recurring for that context specifically. A stable per-context identity closes this
  // permanently rather than narrowing the odds of it, so there is no matching wglDeleteContext-
  // side cache eviction to add either - GITSKey values are never recycled, so there is nothing
  // for a destroyed context's entry to collide with even if it is never removed (mirrors
  // ContextHandleMapService::Erase's own established "player-side cleanup on destroy is not
  // load-bearing within a single replay" precedent - see that method's comment).
  static std::mutex warmedContextsMutex;
  static std::unordered_set<GITSKey> warmedContexts;
  {
    const std::lock_guard lock(warmedContextsMutex);
    if (!warmedContexts.insert(contextKey).second) {
      return; // Already warmed - see this function's own comment above.
    }
  }

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
  // would otherwise rerun per replay (every context-attaching wglMakeCurrent for a genuinely
  // new/different context - the already-warmed check above is what keeps a repeatedly
  // reattached SAME context from paying this cost more than once at all). See
  // generator_player.py's own comment for exactly which commands this excludes (every
  // disabled, mostly-vendor-extension command) and keeps (every enabled one, plus a small,
  // explicit set of disabled commands the player's own hand-written code still resolves
  // through this same dispatch table directly).
  unsigned resolvedCount = 0;
  unsigned totalCount = ${unguarded_warmup_count};
% for guard, count in guarded_warmup_counts.items():
#if ${guard}
  totalCount += ${count};
#endif
% endfor
% for cmd in warmup_commands:
<% guard = platform_guards.get(cmd.api) %>\
% if guard:
#if ${guard}
% endif
  if (g_DispatchTable.${cmd.name}.load(std::memory_order_acquire) != &ResolveAndCall_${cmd.name}) {
    // Already resolved (and cached) by an earlier warm-up or by actual use - the dispatch-table
    // entry no longer points at its default resolve-thunk, so probing again would just repeat
    // a GetProcAddress/wglGetProcAddress call for no benefit: apps that call wglMakeCurrent
    // every frame would otherwise re-probe thousands of
    // functions per frame. Still counts toward the summary line below, so the reported
    // fraction stays accurate.
    ++resolvedCount;
  } else if (void* resolved = ResolveDriverFunction("${cmd.name}", /*logFailure=*/false)) {
    TryCacheRealFunction("${cmd.name}", resolved);
    ++resolvedCount;
  }
% if guard:
#endif
% endif
% endfor

  // Every successful context-attaching wglMakeCurrent call for a genuinely new/different
  // context re-runs the resolution pass above (see WglMakeCurrentRunner::Run(); the
  // already-warmed check at this function's own top is what skips it for a repeatedly
  // reattached SAME context instead) - a later context (e.g. a real context created after a
  // temporary bootstrap one) may resolve additional functions the earlier context could not,
  // so the work itself must still run once per distinct context. Only the aggregate log line
  // is deduplicated on top of that: identical back-to-back counts (the common case once a
  // real application settles onto its one long-lived context) would otherwise print the same
  // line once per newly-warmed context, burying the one INFO line that actually carries
  // information (the resolved count changing).
  static std::mutex logMutex;
  static unsigned lastLoggedCount = 0;
  static bool loggedOnce = false;
  const std::lock_guard lock(logMutex);
  if (!loggedOnce || resolvedCount != lastLoggedCount) {
    LOG_INFO << "Resolved " << resolvedCount << "/" << totalCount
             << " OpenGL functions against the real driver.";
    lastLoggedCount = resolvedCount;
    loggedOnce = true;
  }
}

} // namespace ${namespace}
