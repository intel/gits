// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

${AUTO_GENERATED_HEADER}

#pragma once

#include "layerAuto.h"
#include "subcaptureRange.h"

#include <set>
#include <unordered_set>

namespace gits::opengl {

// Collects the objects the commands of a subcapture range name, and writes them to the analysis
// file when the range ends. Commands compiled into display lists count wherever they are
// compiled, since the range may call the lists.
class AnalyzerLayer : public Layer {
public:
  explicit AnalyzerLayer(const SubcaptureRange& range) : Layer("Analyzer"), m_Range(range) {}

% for name in analyzed:
  void Pre(${name}Command& command) override;
% endfor
  void Pre(MappedDataMetaCommand& command) override;
  void Pre(UniformArrayMetaCommand& command) override;
  void Pre(MappedTextureDataMetaCommand& command) override;
  void Pre(glNewListCommand& command) override;
  void Pre(glEndListCommand& command) override;
% for name in frame_end_commands:
  void Pre(${name}Command& command) override;
  void Post(${name}Command& command) override;
% endfor

private:
  bool Collecting(uint32_t threadId) const {
    return m_Range.InRange() || m_CompilingLists.contains(threadId);
  }
  void Use(GITSKey key) {
    if (key != 0) {
      m_Objects.insert(key);
    }
  }
  template <typename T>
  void Use(const HandleArgument<T>& argument) {
    Use(argument.Key);
  }
  template <typename T>
  void Use(const HandleArrayArgument<T>& argument) {
    for (GITSKey key : argument.Key) {
      Use(key);
    }
  }
  void EndFrame();

  SubcaptureRange m_Range;
  std::unordered_set<GITSKey> m_Objects;
  std::set<uint32_t> m_CompilingLists;
  bool m_SwapEndsFrame{false};
};

} // namespace gits::opengl
