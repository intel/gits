// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerResults.h"
#include "configurator.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace gits {
namespace vulkan {

AnalyzerResults::AnalyzerResults() {
  m_Optimize = Configurator::Get().common.features.subcapture.optimize;

  std::ifstream analysis(GetAnalysisFileName());
  if (!analysis) {
    return;
  }

  std::string token;
  while (analysis >> token) {
    if (token == "OBJECTS") {
      continue;
    }
    try {
      m_ObjectKeys.insert(static_cast<uint64_t>(std::stoull(token)));
    } catch (...) {
      // Ignore malformed tokens; an unreadable entry simply means that key
      // is not in the restore set (RestoreObject handles the empty/partial set).
    }
  }
}

bool AnalyzerResults::RestoreObject(uint64_t objectKey) const {
  if (!m_Optimize || m_ObjectKeys.empty()) {
    return true;
  }
  return m_ObjectKeys.find(objectKey) != m_ObjectKeys.end();
}

bool AnalyzerResults::IsAnalysis() {
  return std::filesystem::exists(GetAnalysisFileName());
}

std::string AnalyzerResults::GetAnalysisFileName() {
  const Configuration& config = Configurator::Get();
  std::stringstream fileName;
  fileName << config.common.player.streamDir.filename().string() << "_frames-"
           << config.common.features.subcapture.frames << "_analysis.txt";
  return fileName.str();
}

} // namespace vulkan
} // namespace gits
