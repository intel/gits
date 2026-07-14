// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerResults.h"
#include "configurator.h"
#include "log.h"

#include "yaml-cpp/yaml.h"

#include <filesystem>
#include <sstream>

namespace gits {
namespace vulkan {

AnalyzerResults::AnalyzerResults() {
  m_Optimize = Configurator::Get().common.player.subcapture.optimize;

  const std::string fileName = GetAnalysisFileName();
  if (!std::filesystem::exists(fileName)) {
    return;
  }
  if (!IsCompleteAnalysisFile(fileName)) {
    LOG_WARNING << "Vulkan subcapture: analysis file '" << fileName
                << "' is incomplete or corrupt (missing completion marker); ignoring it and "
                   "re-running the analysis pass to regenerate it.";
    return;
  }

  try {
    YAML::Node root = YAML::LoadFile(fileName);
    const YAML::Node objects = root["Objects"];
    if (objects && objects.IsSequence()) {
      for (const auto& node : objects) {
        m_ObjectKeys.insert(node.as<uint64_t>());
      }
    }
  } catch (const std::exception& e) {
    // Treat any parse error as no valid analysis: the file is ignored, IsAnalysis()
    // reports false, and the analysis pass re-runs to regenerate it.  (The
    // completion-marker check above already filtered cleanly truncated files.)
    LOG_WARNING << "Vulkan subcapture: failed to parse analysis file '" << fileName << "' ("
                << e.what() << "); ignoring it and re-running the analysis pass to regenerate it.";
    m_ObjectKeys.clear();
  }
}

bool AnalyzerResults::RestoreObject(uint64_t objectKey) const {
  if (!m_Optimize || m_ObjectKeys.empty()) {
    return true;
  }
  return m_ObjectKeys.find(objectKey) != m_ObjectKeys.end();
}

bool AnalyzerResults::IsAnalysis() {
  // A previous analysis pass counts as done only if it produced a *complete*
  // file (carrying the trailing completion marker).  A truncated file from an
  // interrupted analysis run is ignored here so the analysis pass re-runs and
  // regenerates it, instead of silently driving the recording pass with a
  // partial restore set.
  return IsCompleteAnalysisFile(GetAnalysisFileName());
}

bool AnalyzerResults::IsCompleteAnalysisFile(const std::string& fileName) {
  if (!std::filesystem::exists(fileName)) {
    return false;
  }
  try {
    YAML::Node root = YAML::LoadFile(fileName);
    return root["Complete"] && root["Complete"].as<bool>();
  } catch (...) {
    return false;
  }
}

std::string AnalyzerResults::GetAnalysisFileName() {
  const Configuration& config = Configurator::Get();
  std::stringstream fileName;
  fileName << config.common.player.streamDir.filename().string() << "_frames-"
           << config.common.player.subcapture.frames << "_analysis.yml";
  return fileName.str();
}

} // namespace vulkan
} // namespace gits
