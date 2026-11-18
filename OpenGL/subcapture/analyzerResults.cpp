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
#include "subcaptureSession.h"

#include "yaml-cpp/yaml.h"

#include <algorithm>
#include <fstream>
#include <vector>

namespace gits::opengl {

AnalyzerResults::AnalyzerResults() {
  if (!Configurator::Get().common.player.subcapture.optimize) {
    return;
  }
  const std::string fileName = FileName();
  if (!stream::SubcaptureSession::IsAnalysisUsable(fileName, true)) {
    LOG_WARNING << "Subcapture: no complete analysis file '" << fileName
                << "', restoring every object";
    return;
  }
  try {
    for (const auto& node : YAML::LoadFile(fileName)["Objects"]) {
      m_Objects.insert(node.as<GITSKey>());
    }
    m_Loaded = true;
    LOG_INFO << "Subcapture: restoring the contents of objects used by the range, from '"
             << fileName << "'";
  } catch (const std::exception& e) {
    LOG_WARNING << "Subcapture: cannot parse analysis file '" << fileName << "' (" << e.what()
                << "), restoring every object";
    m_Objects.clear();
  }
}

// The subcaptures of a stream with several APIs run their analysis pass together.
bool AnalyzerResults::NeedsAnalysis() {
  const auto& player = Configurator::Get().common.player;
  return !stream::SubcaptureSession::IsAnalysisDone(player.streamDir, player.subcapture.frames,
                                                    player.subcapture.optimize);
}

void AnalyzerResults::Write(const std::unordered_set<GITSKey>& objects) {
  std::vector<GITSKey> sorted(objects.begin(), objects.end());
  std::sort(sorted.begin(), sorted.end());
  YAML::Emitter out;
  out << YAML::BeginMap;
  out << YAML::Key << "Objects" << YAML::Value << sorted;
  out << YAML::Key << "Complete" << YAML::Value << true;
  out << YAML::EndMap;
  const std::string fileName = FileName();
  std::ofstream file(fileName);
  file << out.c_str() << '\n';
  if (!file) {
    LOG_ERROR << "Subcapture: cannot write analysis file '" << fileName << "'";
    return;
  }
  LOG_INFO << "Subcapture: analysis written to '" << fileName << "', run again to record";
}

std::string AnalyzerResults::FileName() {
  const auto& player = Configurator::Get().common.player;
  return stream::SubcaptureSession::AnalysisFileName(player.streamDir, player.subcapture.frames,
                                                     stream::StreamHeader::Api::API_OPENGL);
}
} // namespace gits::opengl
