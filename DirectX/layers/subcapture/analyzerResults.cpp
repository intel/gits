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
    std::string str;
    analysis >> str;
    bool commandLists = true;
    bool commandQueues = false;
    bool objects = false;
    bool descriptors = false;
    bool tlases = false;
    bool blases = false;
    while (analysis >> str) {
      if (str == "COMMAND_QUEUE_COMMANDS") {
        commandLists = false;
        commandQueues = true;
      } else if (str == "OBJECTS") {
        commandQueues = false;
        objects = true;
      } else if (str == "DESCRIPTORS") {
        objects = false;
        descriptors = true;
      } else if (str == "TLASES") {
        descriptors = false;
        tlases = true;
      } else if (str == "BLASES") {
        tlases = false;
        blases = true;
      } else if (str == "AS_SOURCES") {
        blases = false;
      } else {
        unsigned key = std::stoi(str);
        if (commandLists) {
          commandListKeys_.insert(key);
        } else if (commandQueues) {
          commandQueueCommands_.insert(key);
        } else if (objects) {
          objectKeys_.insert(key);
        } else if (descriptors) {
          analysis >> str;
          unsigned index = std::stoi(str);
          descriptors_.insert(std::make_pair(key, index));
        } else if (tlases) {
          tlases_.insert(key);
        } else if (blases) {
          analysis >> str;
          unsigned offset = std::stoi(str);
          blases_.insert(std::make_pair(key, offset));
        } else {
          analysis >> str;
          unsigned offset = std::stoi(str);
          asSources_.insert(std::make_pair(key, offset));
        }
      }
    }
  }
  optimize_ = Configurator::Get().directx.features.subcapture.optimize;
}

bool AnalyzerResults::restoreObject(unsigned objectKey) {
  if (!optimize_ || objectKeys_.empty()) {
    return true;
  }
  return objectKeys_.find(objectKey) != objectKeys_.end();
}

bool AnalyzerResults::restoreDescriptor(unsigned heapKey, unsigned index) {
  if (!optimize_) {
    return true;
  }
  if (descriptors_.empty()) {
    return false;
  }
  return descriptors_.find(std::make_pair(heapKey, index)) != descriptors_.end();
}

bool AnalyzerResults::restoreTlas(unsigned blasBuildKey) {
  return tlases_.find(blasBuildKey) != tlases_.end();
}

bool AnalyzerResults::restoreBlas(std::pair<unsigned, unsigned> blas) {
  if (!optimize_) {
    return true;
  }
  return blases_.find(blas) != blases_.end();
}

bool AnalyzerResults::isAnalysis() {
  return std::filesystem::exists(getAnalysisFileName());
}

std::string AnalyzerResults::getAnalysisFileName() {
  const Config& config = Configurator::Get();
  std::stringstream fileName;
  fileName << config.common.player.streamDir.filename().string() << "_frames-"
           << config.directx.features.subcapture.frames;

  std::string commandListExecutions = config.directx.features.subcapture.commandListExecutions;
  if (!commandListExecutions.empty()) {
    fileName << "_executions_" << commandListExecutions;
  }

  fileName << "_analysis.txt";
  return fileName.str();
}

} // namespace DirectX
} // namespace gits
