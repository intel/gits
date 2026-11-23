// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "unsupportedCommand.h"

#include "captureManager.h"
#include "log.h"
#include "systemApi.h"

#include "tbb/concurrent_unordered_set.h"

#include <string>

namespace gits::opengl {

void ReportUnsupportedCommand(const char* name, uint64_t key) {
  // This runs for every top-level disabled call - currently every GL call in the
  // application, since no command is enabled yet - so it must not serialize threads
  // through a shared mutex; see modern-opengl-backend-architecture.md, "Suppress
  // recursive capture with thread-local depth" (rejects a process-wide GL-call mutex
  // for the same concurrency reason). tbb::concurrent_unordered_set::insert() is
  // thread-safe on its own, so the once-per-name warning below needs no manual lock.
  static tbb::concurrent_unordered_set<std::string> alreadyWarnedNames;
  if (alreadyWarnedNames.insert(name).second) {
    LOG_WARNING << "Unsupported OpenGL command: " << name;
  }

  // Record every occurrence (not just the first), independent of the warning above:
  // see "Intercept every known runtime command, support commands incrementally". `key` was
  // already reserved by the caller before the real driver dispatch - see this function's own
  // declaration comment.
  CaptureManager::Get().RecordUnsupportedCommand(name, key, CurrentThreadId());
}

} // namespace gits::opengl
