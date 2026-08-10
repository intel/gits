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
  // Affects optimized and unoptimized runs
  m_CaptureAsBuildInputs = Configurator::Get().common.player.subcapture.vulkan.captureASBuildInputs;
  if (!m_Optimize) {
    // No analysis-derived behaviour at all, so a file left over from an earlier
    // optimized run must not be read.
    return;
  }

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
    const YAML::Node blasChain = root["BlasChain"];
    if (blasChain && blasChain.IsSequence()) {
      m_BlasChainLoaded = true;
      for (const auto& node : blasChain) {
        BlasChainOp op;
        // Throws exception when key is not found
        op.CommandKey = node["Cmd"].as<uint64_t>();
        op.DstAsKey = node["DstAs"].as<uint64_t>();
        op.SourceCommandKey = node["SrcCmd"].as<uint64_t>();
        op.SrcAsKey = node["SrcAs"] ? node["SrcAs"].as<uint64_t>() : 0;
        op.IsCopy = node["IsCopy"].as<int>() != 0;
        if (op.IsCopy) {
          // Required for a copy. A missing one would silently mean MODE_CLONE.
          op.CopyMode = static_cast<VkCopyAccelerationStructureModeKHR>(node["CopyMode"].as<int>());
        }
        m_RetainedBlasCommands.insert(op.CommandKey);
        if (op.SourceCommandKey) {
          m_BlasSourceByCommand[op.CommandKey] = op.SourceCommandKey;
        }
        m_BlasChain.push_back(op);
      }
    }
  } catch (const std::exception& e) {
    // Treat any parse error as no valid analysis: nothing is loaded, so the recording
    // pass restores everything rather than acting on half-read data. IsAnalysis() only
    // tests the completion marker, so pass selection is unchanged and a raytracing
    // stream stops at SubcaptureLayer::RequireBlasChainForRaytracing.
    LOG_WARNING << "Vulkan subcapture: failed to parse analysis file '" << fileName << "' ("
                << e.what() << "); ignoring it - delete it to re-run the analysis pass.";
    m_ObjectKeys.clear();
    m_BlasChainLoaded = false;
    m_BlasChain.clear();
    m_RetainedBlasCommands.clear();
    m_BlasSourceByCommand.clear();
  }
}

bool AnalyzerResults::RestoreObject(uint64_t objectKey) const {
  if (!m_Optimize || m_ObjectKeys.empty()) {
    return true;
  }
  return m_ObjectKeys.find(objectKey) != m_ObjectKeys.end();
}

bool AnalyzerResults::RestoreBlasCommand(uint64_t commandKey) const {
  // Fall back to restoring everything when optimization is off or no BlasChain was
  // loaded, matching the legacy per-AS last-build restore behaviour.
  if (!m_Optimize || !m_BlasChainLoaded) {
    return true;
  }
  return m_RetainedBlasCommands.find(commandKey) != m_RetainedBlasCommands.end();
}

uint64_t AnalyzerResults::GetBlasSourceCommand(uint64_t commandKey) const {
  auto it = m_BlasSourceByCommand.find(commandKey);
  return it == m_BlasSourceByCommand.end() ? 0 : it->second;
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
