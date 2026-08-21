// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once
#include "arguments.h"
#include "command.h"
#include "hashUtils.h"

#include <string>
#include <unordered_set>
#include <unordered_map>

namespace gits {
namespace DirectX {

class AnalyzerResults {
public:
  AnalyzerResults();

  bool RestoreCommandLists() {
    return !m_CommandListKeys.empty();
  }
  bool RestoreCommandList(GITSKey commandListKey) {
    return m_CommandListKeys.find(commandListKey) != m_CommandListKeys.end();
  }
  bool RestoreCommandQueueCommand(CommandKey commandKey) {
    return m_CommandQueueCommands.find(commandKey) != m_CommandQueueCommands.end();
  }
  bool RestoreObject(GITSKey objectKey);
  bool RestoreDescriptor(GITSKey heapKey, unsigned index);
  bool RestoreTlas(CommandKey buildKey);
  bool RestoreBlas(CommandKey buildKey);
  CommandKey GetBlasSourceBuild(CommandKey buildKey);

  static bool IsAnalysis();
  static std::string GetAnalysisFileName();

private:
  bool m_Optimize{};
  std::unordered_set<GITSKey> m_CommandListKeys;
  std::unordered_set<CommandKey> m_CommandQueueCommands;
  std::unordered_set<GITSKey> m_ObjectKeys;
  std::unordered_set<std::pair<GITSKey, unsigned>, UnsignedPairHash> m_Descriptors;
  std::unordered_set<CommandKey> m_Tlases;
  std::unordered_map<CommandKey, CommandKey> m_Blases;
};

} // namespace DirectX
} // namespace gits
