// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

#pragma once

#include <string>
#include <unordered_set>
#include <set>

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
  bool RestoreTlas(unsigned blasBuildKey);
  bool RestoreBlas(std::pair<unsigned, unsigned> blas);
  std::set<std::pair<unsigned, unsigned>>& GetAsSources() {
    return m_AsSources;
  }

  static bool IsAnalysis();
  static std::string GetAnalysisFileName();

private:
  bool m_Optimize{};
  std::unordered_set<unsigned> m_CommandListKeys;
  std::unordered_set<unsigned> m_CommandQueueCommands;
  std::unordered_set<unsigned> m_ObjectKeys;
  std::set<std::pair<unsigned, unsigned>> m_Descriptors;
  std::unordered_set<unsigned> m_Tlases;
  std::set<std::pair<unsigned, unsigned>> m_Blases;
  std::set<std::pair<unsigned, unsigned>> m_AsSources;
};

} // namespace DirectX
} // namespace gits
