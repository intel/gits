// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#include "analyzerResults.h"
#include "arguments.h"
#include "command.h"
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
      } else {
        if (commandLists) {
          GITSKey key = std::stoull(str);
          m_CommandListKeys.insert(key);
        } else if (commandQueues) {
          CommandKey key = std::stoull(str);
          m_CommandQueueCommands.insert(key);
        } else if (objects) {
          GITSKey key = std::stoull(str);
          m_ObjectKeys.insert(key);
        } else if (descriptors) {
          GITSKey key = std::stoull(str);
          analysis >> str;
          unsigned index = std::stoi(str);
          m_Descriptors.insert(std::make_pair(key, index));
        } else if (tlases) {
          CommandKey key = std::stoull(str);
          m_Tlases.insert(key);
        } else if (blases) {
          CommandKey key = std::stoull(str);
          analysis >> str;
          CommandKey source = std::stoull(str);
          m_Blases.insert(std::make_pair(key, source));
        }
      }
    }
  }
  m_Optimize = Configurator::Get().common.player.subcapture.optimize;
}

bool AnalyzerResults::RestoreObject(GITSKey objectKey) {
  if (!m_Optimize || m_ObjectKeys.empty()) {
    return true;
  }
  return m_ObjectKeys.find(objectKey) != m_ObjectKeys.end();
}

bool AnalyzerResults::RestoreDescriptor(GITSKey heapKey, unsigned index) {
  if (!m_Optimize) {
    return true;
  }
  if (m_Descriptors.empty()) {
    return false;
  }
  return m_Descriptors.find(std::make_pair(heapKey, index)) != m_Descriptors.end();
}

bool AnalyzerResults::RestoreTlas(CommandKey buildKey) {
  return m_Tlases.contains(buildKey);
}

bool AnalyzerResults::RestoreBlas(CommandKey buildKey) {
  if (!m_Optimize) {
    return true;
  }
  return m_Blases.contains(buildKey);
}

CommandKey AnalyzerResults::GetBlasSourceBuild(CommandKey buildKey) {
  auto it = m_Blases.find(buildKey);
  if (it != m_Blases.end()) {
    return it->second;
  }
  return CommandKey{};
}

bool AnalyzerResults::IsAnalysis() {
  return std::filesystem::exists(GetAnalysisFileName());
}

std::string AnalyzerResults::GetAnalysisFileName() {
  const Configuration& config = Configurator::Get();
  std::stringstream fileName;
  fileName << config.common.player.streamDir.filename().string() << "_frames-"
           << config.common.player.subcapture.frames;

  std::string commandListExecutions = config.common.player.subcapture.directx.commandListExecutions;
  if (!commandListExecutions.empty()) {
    fileName << "_executions_" << commandListExecutions;
  }

  fileName << "_analysis.txt";
  return fileName.str();
}

} // namespace DirectX
} // namespace gits
