// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "traceFiles.h"

#include "configurationLib.h"

#include <string>

namespace gits {

namespace {

std::unique_ptr<FastOStream> OpenTraceStream(bool enabled, bool ipc, const std::string& path) {
  if (!enabled) {
    return std::make_unique<FastOFileStream>();
  }
  if (ipc) {
    return std::make_unique<FastOStringStream>(path);
  }
  return std::make_unique<FastOFileStream>(path);
}

} // namespace

std::shared_ptr<TraceFiles> TraceFiles::Acquire(const std::filesystem::path& base) {
  static std::mutex mutex;
  static std::weak_ptr<TraceFiles> current;
  std::lock_guard<std::mutex> lock(mutex);
  if (auto files = current.lock()) {
    return files;
  }
  std::shared_ptr<TraceFiles> files(new TraceFiles(base));
  current = files;
  return files;
}

TraceFiles::TraceFiles(const std::filesystem::path& base) {
  const std::string stem = base.string() + "_tracefile";
  const std::string extension = ".txt";
  std::string suffix;
  for (unsigned index = 1; std::filesystem::exists(stem + suffix + extension) ||
                           std::filesystem::exists(stem + "_pre" + suffix + extension);
       ++index) {
    suffix = std::to_string(index);
  }

  const auto& trace = Configurator::Get().common.shared.trace;
  const bool ipc = trace.flushMethod == "ipc";
  m_Pre = OpenTraceStream(trace.print.preCalls, ipc, stem + "_pre" + suffix + extension);
  m_Post = OpenTraceStream(trace.print.postCalls, ipc, stem + suffix + extension);
}

TraceFiles::~TraceFiles() = default;

} // namespace gits
