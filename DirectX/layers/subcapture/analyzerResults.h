// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

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
  bool RestoreCommandList(unsigned commandListKey) {
    return m_CommandListKeys.find(commandListKey) != m_CommandListKeys.end();
  }
  bool RestoreCommandQueueCommand(unsigned commandKey) {
    return m_CommandQueueCommands.find(commandKey) != m_CommandQueueCommands.end();
  }
  bool RestoreObject(unsigned objectKey);
  bool RestoreDescriptor(unsigned heapKey, unsigned index);
  bool RestoreTlas(unsigned buildKey);
  bool RestoreBlas(unsigned buildKey);
  unsigned GetBlasSourceBuild(unsigned buildKey);

  static bool IsAnalysis();
  static std::string GetAnalysisFileName();

private:
  bool m_Optimize{};
  std::unordered_set<unsigned> m_CommandListKeys;
  std::unordered_set<unsigned> m_CommandQueueCommands;
  std::unordered_set<unsigned> m_ObjectKeys;
  std::unordered_set<std::pair<unsigned, unsigned>, UnsignedPairHash> m_Descriptors;
  std::unordered_set<unsigned> m_Tlases;
  std::unordered_map<unsigned, unsigned> m_Blases;
};

} // namespace DirectX
} // namespace gits
