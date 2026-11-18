// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

// Host-only tests for the analysis files a recording pass can use. The APIs of a stream decide
// on them together, so an API accepting a file another API rejects records a subcapture without
// the other API.

#include "subcaptureSession.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

using gits::stream::SubcaptureSession;

int g_Failures = 0;

void Check(bool condition, const char* what) {
  if (!condition) {
    std::printf("FAIL: %s\n", what);
    ++g_Failures;
  }
}

std::string WriteFile(const std::filesystem::path& dir,
                      const char* name,
                      const std::string& contents) {
  const std::filesystem::path path = dir / name;
  std::ofstream(path) << contents;
  return path.string();
}

} // namespace

int main() {
  const std::filesystem::path dir =
      std::filesystem::temp_directory_path() / "gits_subcapture_analysis_test";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);

  const std::string missing = (dir / "missing.yml").string();
  Check(!SubcaptureSession::IsAnalysisUsable(missing, false), "missing file, unoptimized");
  Check(!SubcaptureSession::IsAnalysisUsable(missing, true), "missing file, optimized");

  const std::string incomplete = WriteFile(dir, "incomplete.yml", "Objects: [1, 2]\n");
  Check(!SubcaptureSession::IsAnalysisUsable(incomplete, false), "incomplete file");

  const std::string interrupted =
      WriteFile(dir, "interrupted.yml", "Objects: [1, 2]\nComplete: false\n");
  Check(!SubcaptureSession::IsAnalysisUsable(interrupted, false), "interrupted analysis");

  const std::string corrupt = WriteFile(dir, "corrupt.yml", "Complete: [\n");
  Check(!SubcaptureSession::IsAnalysisUsable(corrupt, false), "corrupt file");

  const std::string full = WriteFile(dir, "full.yml", "Objects: [1, 2]\nComplete: true\n");
  Check(SubcaptureSession::IsAnalysisUsable(full, false), "full analysis, unoptimized");
  Check(SubcaptureSession::IsAnalysisUsable(full, true), "full analysis, optimized");

  const std::string optimized = WriteFile(dir, "optimized.yml", "Optimize: true\nComplete: true\n");
  Check(SubcaptureSession::IsAnalysisUsable(optimized, false), "optimized analysis, unoptimized");
  Check(SubcaptureSession::IsAnalysisUsable(optimized, true), "optimized analysis, optimized");

  const std::string unoptimized =
      WriteFile(dir, "unoptimized.yml", "Optimize: false\nComplete: true\n");
  Check(SubcaptureSession::IsAnalysisUsable(unoptimized, false),
        "unoptimized analysis, unoptimized");
  Check(!SubcaptureSession::IsAnalysisUsable(unoptimized, true), "unoptimized analysis, optimized");

  std::filesystem::remove_all(dir);
  if (g_Failures == 0) {
    std::printf("All subcapture analysis tests passed\n");
  }
  return g_Failures == 0 ? 0 : 1;
}
