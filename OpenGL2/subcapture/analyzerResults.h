// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include "arguments.h"

#include <string>
#include <unordered_set>

namespace gits::opengl2 {

// The analysis file of an optimized subcapture: the objects the range names. The analysis pass
// writes it, and the recording pass restores the contents of only those objects and of the
// objects they depend on at the start of the range.
class AnalyzerResults {
public:
  // Loads the analysis file when Common.Player.Subcapture.Optimize is set.
  AnalyzerResults();

  // True when the restore is limited to the analyzed objects.
  bool Optimize() const {
    return m_Loaded;
  }
  const std::unordered_set<GITSKey>& Objects() const {
    return m_Objects;
  }

  // True when Optimize is set and no complete analysis file exists yet.
  static bool NeedsAnalysis();
  static void Write(const std::unordered_set<GITSKey>& objects);

private:
  static std::string FileName();
  static bool IsComplete(const std::string& fileName);

  bool m_Loaded{false};
  std::unordered_set<GITSKey> m_Objects;
};

} // namespace gits::opengl2
