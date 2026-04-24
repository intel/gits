// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerResults.h"
#include "log.h"
#include "configurator.h"

#include <filesystem>
#include <fstream>

namespace gits {
namespace DirectX {

AnalyzerResults::AnalyzerResults() {
  std::ifstream analysis(GetAnalysisFileName());
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
          m_CommandListKeys.insert(key);
        } else if (commandQueues) {
          m_CommandQueueCommands.insert(key);
        } else if (objects) {
          m_ObjectKeys.insert(key);
        } else if (descriptors) {
          analysis >> str;
          unsigned index = std::stoi(str);
          m_Descriptors.insert(std::make_pair(key, index));
        } else if (tlases) {
          m_Tlases.insert(key);
        } else if (blases) {
          analysis >> str;
          unsigned offset = std::stoi(str);
          m_Blases.insert(std::make_pair(key, offset));
        } else {
          analysis >> str;
          unsigned offset = std::stoi(str);
          m_AsSources.insert(std::make_pair(key, offset));
        }
      }
    }
  }
  m_Optimize = Configurator::Get().directx.features.subcapture.optimize;
}

bool AnalyzerResults::RestoreObject(unsigned objectKey) {
  if (!m_Optimize || m_ObjectKeys.empty()) {
    return true;
  }
  return m_ObjectKeys.find(objectKey) != m_ObjectKeys.end();
}

bool AnalyzerResults::RestoreDescriptor(unsigned heapKey, unsigned index) {
  if (!m_Optimize) {
    return true;
  }
  if (m_Descriptors.empty()) {
    return false;
  }
  return m_Descriptors.find(std::make_pair(heapKey, index)) != m_Descriptors.end();
}

bool AnalyzerResults::RestoreTlas(unsigned blasBuildKey) {
  return m_Tlases.find(blasBuildKey) != m_Tlases.end();
}

bool AnalyzerResults::RestoreBlas(std::pair<unsigned, unsigned> blas) {
  if (!m_Optimize) {
    return true;
  }
  return m_Blases.find(blas) != m_Blases.end();
}

bool AnalyzerResults::IsAnalysis() {
  return std::filesystem::exists(GetAnalysisFileName());
}

std::string AnalyzerResults::GetAnalysisFileName() {
  const Configuration& config = Configurator::Get();
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
