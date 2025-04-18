// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerResults.h"
#include "gits.h"

#include <filesystem>
#include <fstream>

namespace gits {
namespace DirectX {

AnalyzerResults::AnalyzerResults() {
  std::ifstream analysis(getAnalysisFileName());
  if (analysis) {
    std::string line;
    analysis >> line;
    bool commandLists = true;
    while (analysis >> line) {
      if (line == "COMMAND_QUEUE_COMMANDS") {
        commandLists = false;
      } else {
        unsigned key = std::stoi(line);
        if (commandLists) {
          commandListKeys_.insert(key);
        } else {
          commandQueueCommands_.insert(key);
        }
      }
    }
  }
}

bool AnalyzerResults::isAnalysis() {
  return std::filesystem::exists(getAnalysisFileName());
}

std::string AnalyzerResults::getAnalysisFileName() {
  const Config& config = Config::Get();
  std::stringstream fileName;
  fileName << config.common.player.streamDir.filename().string() << "_frames-"
           << config.directx.features.subcapture.frames;
  fileName << "_analysis.txt";
  return fileName.str();
}

} // namespace DirectX
} // namespace gits
