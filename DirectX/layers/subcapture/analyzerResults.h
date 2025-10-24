// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2025 Intel Corporation
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

  bool restoreCommandLists() {
    return !commandListKeys_.empty();
  }
  bool restoreCommandList(unsigned commandListKey) {
    return commandListKeys_.find(commandListKey) != commandListKeys_.end();
  }
  bool restoreCommandQueueCommand(unsigned commandKey) {
    return commandQueueCommands_.find(commandKey) != commandQueueCommands_.end();
  }
  bool restoreObject(unsigned objectKey);
  bool restoreDescriptor(unsigned heapKey, unsigned index);
  bool restoreTlas(unsigned blasBuildKey);
  bool restoreBlas(std::pair<unsigned, unsigned> blas);

  static bool isAnalysis();
  static std::string getAnalysisFileName();

private:
  bool optimize_{};
  std::unordered_set<unsigned> commandListKeys_;
  std::unordered_set<unsigned> commandQueueCommands_;
  std::unordered_set<unsigned> objectKeys_;
  std::set<std::pair<unsigned, unsigned>> descriptors_;
  std::unordered_set<unsigned> tlases_;
  std::set<std::pair<unsigned, unsigned>> blases_;
};

} // namespace DirectX
} // namespace gits
