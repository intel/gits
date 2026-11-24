<%
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

#include "dispatchWarmUp.h"
#include "glDispatchAuto.h"
#include "log.h"

#include <mutex>
#include <unordered_set>

namespace gits::opengl::player {

void WarmUpResolvableFunctions(GITSKey contextKey) {
  // Stable keys are never reused, unlike native context handles.
  static std::mutex warmedContextsMutex;
  static std::unordered_set<GITSKey> warmedContexts;
  {
    const std::lock_guard lock(warmedContextsMutex);
    if (!warmedContexts.insert(contextKey).second) {
      return; // Already warmed - see this function's own comment above.
    }
  }

  // Probe only commands replay may call, while a context is current. Misses keep
  // their lazy thunks, so a later context or an actual call can retry.
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
  {
    const bool alreadyResolved =
        g_DispatchTable.${cmd.name}.load(std::memory_order_acquire) != &ResolveAndCall_${cmd.name};
    const bool resolvedNow = !alreadyResolved && TryResolveRealFunction("${cmd.name}");
    if (alreadyResolved || resolvedNow) {
      ++resolvedCount;
    }
  }
% if guard:
#endif
% endif
% endfor

  // Run once per context even when only the aggregate log line is redundant.
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

} // namespace gits::opengl::player
