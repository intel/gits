// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "fastOStream.h"

#include <filesystem>
#include <memory>
#include <mutex>

namespace gits {

// The trace files of the process. Every API traced by the process writes to the same files, so the
// commands of all APIs appear in one file in execution order. Writers lock Mutex().
class TraceFiles {
public:
  // The first call opens <base>_tracefile[N].txt and <base>_tracefile_pre[N].txt as enabled by
  // Common.Shared.Trace.Print, with the lowest N that names no existing file. Later calls return
  // the open files and ignore base. The files close when the last owner releases them.
  static std::shared_ptr<TraceFiles> Acquire(const std::filesystem::path& base);

  ~TraceFiles();
  TraceFiles(const TraceFiles&) = delete;
  TraceFiles& operator=(const TraceFiles&) = delete;

  // A stream that is disabled by the configuration is not open.
  FastOStream& Pre() {
    return *m_Pre;
  }
  FastOStream& Post() {
    return *m_Post;
  }
  std::mutex& Mutex() {
    return m_Mutex;
  }

private:
  explicit TraceFiles(const std::filesystem::path& base);

  std::unique_ptr<FastOStream> m_Pre;
  std::unique_ptr<FastOStream> m_Post;
  std::mutex m_Mutex;
};

} // namespace gits
